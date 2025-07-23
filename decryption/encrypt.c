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

//This func checks if message in pipe is subscription or guess
char* handle_pipe_message(char *msg, int *decrypters_count, int* id) {
    const char *prefix = "/mnt/mta/decrypter_pipe_";
    size_t prefix_len = strlen(prefix);
    
    //Incase of password message 
    char *space = strchr(msg, ' ');

    size_t path_len = space ? (size_t)(space - msg) : strlen(msg); //Address length
    strncpy(decrypters_pipes[*decrypters_count], msg, sizeof(decrypters_pipes[0]) - 1);
    decrypters_pipes[*decrypters_count][path_len] = '\0';

    if(space != NULL) {
        char *guess = space + 1;
        return guess;
    }
    else {
        (*id) = atoi(decrypters_pipes[*decrypters_count] + prefix_len);
        print_connection((*id), decrypters_pipes[*decrypters_count]);
        (*decrypters_count)++;
        char* str = "pipe subscription request";
        return str;
    }
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
    int decrypters_count = 0;

    while (running) {
        char buf[MAX_REG_MSG];
        ssize_t bytes;
        char subscription_request = false;

        //Reads from pipe and get encrypter pipe data
        while ((bytes = read(pipe_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[bytes] = '\0';
            buf[strcspn(buf, "\n")] = '\0';
            int id;

            char* curr_guess = handle_pipe_message(buf, &decrypters_count, &id);
            if (strcmp(curr_guess, "pipe subscription request") == 0)
                subscription_request = true;
            else 
            {
                pthread_mutex_lock(&shared.guess_mutex);
                memcpy(shared.guess, curr_guess, password_length);
                shared.guesser_id = id;
                shared.guess_pending = true;
                pthread_cond_broadcast(&shared.guess_cond);
                pthread_mutex_unlock(&shared.guess_mutex);
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
            
            bool password_sent[MAX_DECRYPTERS] = {false};
            
            //Send password to decrypter pipe
            if(subscription_request) {
                for(int i = 0; i < decrypters_count; i++){
                    int fd = open(decrypters_pipes[i], O_WRONLY | O_NONBLOCK);
                    if (fd != -1) {
                        write(fd, encrypted, password_length);
                        close(fd);
                    } 
                    else {
                        perror("open decrypter pipe failed");
                        printf("errno: %d (%s)\n", errno, strerror(errno));
                    }
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

                // Send new encrypted password to all decrypter's pipes
                for (int i = 0; i < decrypters_count; i++) {
                    int fd = open(decrypters_pipes[i], O_WRONLY | O_NONBLOCK);
                    if (fd != -1) {
                        write(fd, encrypted, password_length);
                        close(fd);
                    } else {
                        perror("open decrypter pipe failed (post-success)");
                        printf("errno: %d (%s)\n", errno, strerror(errno));
                    }
                }
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