#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <mta_crypt.h> 
#include <mta_rand.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> 
#include <errno.h> 
#include <sys/stat.h>
#include "shared.h"
#include "encrypt.h"

//Global variables
extern SharedData shared;
extern int password_length;
extern int num_decrypters;
extern int timeout_seconds;
extern bool running;

#define MAX_REG_MSG 128
#define MAX_DECRYPTERS 100

char decrypters_pipes[MAX_DECRYPTERS][128];
int decrypters_count = 0;

//This function generates a printable password and writes it into the provided buffer; Helper function
void generate_printable_password(char *password, int length) {

    for (int i = 0; i < length; ++i) {
        char c;
        do { c = MTA_get_rand_char(); }
        while (!isprint(c));
        password[i] = c;
    }
}

//This function encrypts a password using MTA encryption with the given key and key length
void encrypt_password(char *password,char *key, char *encrypted, int length, int key_length) {
    unsigned int out_len = length;
    MTA_encrypt(key,key_length, password, length, encrypted, &out_len);
}

//This function prints a log when a new password is generated
void print_new_pw(char* password, char* key, char* encrypted) {
    printf("%ld\t[ENCRYPTER]\t[INFO]\tNew password generated: %.*s, key: %.*s, After encryption: %.*s\n",
    time(NULL),
    password_length, password,
    password_length/8, key,
    password_length, encrypted);
}

//This function prints a success log when a correct password guess is received from a decrypter
void print_success(int decrypter_id, char* password) {
    printf("%ld\t[ENCRYPTER]\t[OK]\tPassword decrypted successfully by client #%d, received (%.*s), is (%.*s)\n",
    time(NULL),
    decrypter_id,
    password_length, shared.guess,
    password_length, password);
}


//This function prints an error log when no password guess is received within the configured timeout
void print_timeout() {
    printf("%ld\t[ENCRYPTER]\t[ERROR]\tNo password received during configured timeout period (%d seconds), regenerating password\n",
    time(NULL),
    timeout_seconds);
}

//This function prints a log when a connection request occured
void print_connection(int decrypter_id, char* fifo_path) {
    printf("%ld\t[ENCRYPTER]\t[INFO]\tReceived connection request from decrypter id %d, fifo name %s\n\n",
    time(NULL),
    decrypter_id,
    fifo_path);
}

//This function prints a log when a decrypter submits a correct but outdated password
void print_old_pw_guess(int decrypter_id, char* guess) {
    printf("%ld\t[ENCRYPTER]\t[ERROR]\tReceived correct but outdated password from client #%d: (%.*s)\n",
    time(NULL),
    decrypter_id,
    password_length, guess);
}

//This function prints an error log when a wrong password guess is submitted by a decrypter
void print_wrong_guess(int decrypter_id, char* guess, char* password) {
    printf("%ld\t[ENCRYPTER]\t[ERROR]\tWrong password received from client #%d (%.*s), should be (%.*s)\n",
    time(NULL),
    decrypter_id,
    password_length, guess,
    password_length, password);
}

//This function creates the main named pipe
void create_main_pipe() {
    const char* pipe_path = "/mnt/mta/encrypter_pipe";

    //In case it already exsists
    if (mkfifo(pipe_path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    printf("Created main pipe: %s\n", pipe_path);
}

//This function is executed by the encrypter thread, and coordinates password generation and validation; Encrypter thread function
void *encrypter(void *arg) {
    bool first = true;

    create_main_pipe();

    int pipe_fd = open("/mnt/mta/encrypter_pipe", O_RDWR  | O_NONBLOCK); //Opens pipe for read and write
    if (pipe_fd == -1) {
        perror("open pipe");
        exit(1);
    }

    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];

    while (running) {
        int id = -1;
        char buf[MAX_REG_MSG];
        ssize_t bytes;
        if ((bytes = read(pipe_fd, buf, sizeof(buf) - 1)) > -1) {//reads from pipe
            if (strncmp(buf, "id:", 3) == 0) {
                buf[bytes] = '\0';
                id = atoi(buf + 3);
                snprintf(decrypters_pipes[id], sizeof(decrypters_pipes[0]), "/mnt/mta/decrypter_pipe_%d", id);
                print_connection(id, decrypters_pipes[id]);
                fflush(stdout);
            }
        }
        
        pthread_mutex_lock(&shared.mutex);
        if (first || shared.decrypted) {
            first = false;
            shared.decrypted = false; //Alert the new password going to be encrypted is not yet decrypted

            //Copy previous password before regenerating
            memcpy(shared.previous_password, password, password_length);
            pthread_mutex_unlock(&shared.mutex);

            generate_printable_password(password, password_length);
            MTA_get_rand_data(key, password_length / 8);
            encrypt_password(password, key, encrypted, password_length, password_length / 8);

            //Send to decrypter pipe
            if(id != -1){
                int fd = open(decrypters_pipes[id], O_WRONLY | O_NONBLOCK);
                if (fd != -1) {
                    write(fd, encrypted, password_length);
                    close(fd);
                } else {
                    perror("open decrypter pipe failed");
                }
            }

            //Write encrypted password to shared buffer
            pthread_mutex_lock(&shared.mutex);
            memcpy(shared.encrypted, encrypted, password_length);
            shared.length = password_length;
            shared.new_data = true; //New encrypted password available
            shared.guess_pending = false;
            pthread_cond_broadcast(&shared.cond);
            pthread_mutex_unlock(&shared.mutex);

            print_new_pw(password, key, encrypted); //Prints new password info
        }
        else
            pthread_mutex_unlock(&shared.mutex);
        
        time_t start = time(NULL);
        pthread_mutex_lock(&shared.guess_mutex);

        if (timeout_seconds > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_seconds - (time(NULL) - start);
            
            while (!shared.guess_pending && !shared.decrypted) {
                int rc = pthread_cond_timedwait(&shared.guess_cond, &shared.guess_mutex, &ts);
            
                if (rc == ETIMEDOUT) { //Case: timeout
                    print_timeout();
                    shared.decrypted = true; //Simulate new password
                    pthread_cond_broadcast(&shared.guess_cond);
                    break;
                }
            }   
        }
        else //Case: no timeout, AKA -t flag wasn't given
            while (!shared.guess_pending && !shared.decrypted)
                pthread_cond_wait(&shared.guess_cond, &shared.guess_mutex);

        //If a guess is pending, handle
        if (shared.guess_pending) {
            int decrypter_id = shared.guesser_id;
            char guess_curr[MAX_PASSWORD_LENGTH] = {};
            memcpy(guess_curr, shared.guess, password_length);

            //Mark the slot free for the next guess
            shared.guess_pending = false;
            pthread_cond_broadcast(&shared.guess_cond);
            pthread_mutex_unlock(&shared.guess_mutex);

            bool match = (memcmp(guess_curr, password, password_length) == 0);
            pthread_mutex_lock(&shared.mutex);
            if (match && !shared.decrypted) {
                shared.decrypted = true;
                print_success(decrypter_id, password);
                pthread_cond_broadcast(&shared.cond);
            }
            else if (match) //Case: old guess
                print_old_pw_guess(decrypter_id, guess_curr);
            else {
                bool old_match = (memcmp(guess_curr, shared.previous_password, password_length) == 0);
                if (old_match) //Case: Old password guess
                    print_old_pw_guess(decrypter_id, guess_curr);
                else //Case: Wrong guess
                    print_wrong_guess(decrypter_id, guess_curr, password);
            }
            pthread_mutex_unlock(&shared.mutex);
        }
        else
            pthread_mutex_unlock(&shared.guess_mutex);
            
    }
    return NULL;
}