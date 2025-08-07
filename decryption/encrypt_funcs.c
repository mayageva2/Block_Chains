#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <mta_crypt.h> 
#include <mta_rand.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> 
#include <errno.h> 
#include <stdbool.h>
#include <sys/stat.h>
#include "encrypt_funcs.h"

#define MAX_REG_MSG 128
#define MAX_DECRYPTERS 100
#define MAX_PASSWORD_LENGTH 1024
#define MAX_PATH_LEN 30

char decrypters_pipes[MAX_DECRYPTERS][128];

typedef struct {
    int decrypter_id;
    char guess[MAX_PASSWORD_LENGTH];
    bool is_pending;
} GuessState;

//This function generates a printable password and writes it into the provided buffer; Helper function
void generate_printable_password(char *password, int length) {

    for (int i = 0; i < length; ++i) {
        char c;
        do {c=MTA_get_rand_char();}
        while(!isprint(c));
        password[i]=c;
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
void print_success(int decrypter_id, char* password, char* guess) {
    printf("%ld\t[ENCRYPTER]\t[OK]\tPassword decrypted successfully by client #%d, received (%.*s), is (%.*s)\n",
    time(NULL),
    decrypter_id,
    password_length, guess,
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
    
    //Write msg to decryptor pipe
    void send_msg_to_decryptor_pipe(const char* decrypter_pipe, char* msg, int len)
    {
        int fd = open(decrypter_pipe, O_WRONLY); 
        if (fd != -1) {
            write(fd, msg, len);
            close(fd);
        } 
        else {
            if (errno == ENOENT || errno == ENXIO) {return;}
            perror("open decrypter pipe failed");
            printf("errno: %d (%s)\n", errno, strerror(errno));
        }
    }
    
    //This func checks if message in pipe is subscription or guess
    char* handle_pipe_message(char *msg, int* decrypters_count, int* id) {
        const char *prefix = "/mnt/mta/decrypter_pipe_";
        size_t prefix_len = strlen(prefix);
        
        //Incase of password message 
        char *space = strchr(msg, ' ');
        size_t path_len = space ? (size_t)(space - msg) : strlen(msg); //Address length
    
        if(space != NULL) {
           for (int i = 0; i < *decrypters_count; i++) {
                if (strncmp(decrypters_pipes[i], msg, path_len) == 0) {
                    *id = atoi(decrypters_pipes[i] + prefix_len);
                    break;
                }
            }
            char *guess = space + 1;
            return guess;
        }
        else {
            strncpy(decrypters_pipes[*decrypters_count], msg, sizeof(decrypters_pipes[0]) - 1);
            decrypters_pipes[*decrypters_count][sizeof(decrypters_pipes[0]) - 1] = '\0';
            (*id) = atoi(decrypters_pipes[*decrypters_count] + prefix_len);
            print_connection((*id), decrypters_pipes[*decrypters_count]);
            (*decrypters_count)++;
            return "pipe subscription request"; 
        }
    }
    
    //This func reads decrypter msg and sends encrypted password to decrypter
    void read_encrypter_pipe_data(int pipe_fd, int *decrypters_count, GuessState* current_guess, char* encrypted) {
        ssize_t bytes = 0;
        bool subscription_request = false;
        char buf[MAX_REG_MSG];
    
        //Reads from pipe and get encrypter pipe data
        while ((bytes = read(pipe_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[bytes] = '\0';
            buf[strcspn(buf, "\n")] = '\0';
            int id;
            char* curr_guess = handle_pipe_message(buf, decrypters_count, &id);
            if (strcmp(curr_guess, "pipe subscription request") == 0)
                subscription_request = true;
            else 
            {
                subscription_request = false;
                memcpy(current_guess->guess, curr_guess, password_length);
                current_guess->decrypter_id = id;
                current_guess->is_pending = true;
            }
        }
    
        //Send password to decrypter pipe
        if(subscription_request) {
            for(int i = 0; i < *decrypters_count; i++){
                send_msg_to_decryptor_pipe(decrypters_pipes[i], encrypted, password_length);
            }
            subscription_request = false;
        }
    }
    
    //This func checks if decrypter guess is correct or not and send response
    bool check_guess(GuessState* current_guess, char* password, bool* password_decrypted, char* prev_password ) {
        int decrypter_id = current_guess->decrypter_id;
        char guess_curr[MAX_PASSWORD_LENGTH] = {};
        memcpy(guess_curr, current_guess->guess, password_length);
        current_guess->is_pending = false;
    
        char response_pipe[MAX_PATH_LEN];
        bool match = (memcmp(guess_curr, password, password_length) == 0);
        bool old_match = (memcmp(guess_curr, prev_password, password_length) == 0);
    
        snprintf(response_pipe, sizeof(response_pipe), "/mnt/mta/decrypter_pipe_%d", decrypter_id);
    
        if (match && !(*password_decrypted)) {
            *password_decrypted = true;
            print_success(decrypter_id, password, guess_curr);
            return true;
        } else {
            if (match || old_match) {
                print_old_pw_guess(decrypter_id, guess_curr);
            } else {
                print_wrong_guess(decrypter_id, guess_curr, password);
            }
            return false;
        }
    }
    
 
//This function is executed by the encrypter thread, and coordinates password generation and validation; Encrypter thread function
void *encrypter(void *arg) {
    bool first = true;
    GuessState current_guess = {.is_pending = false};
    bool password_decrypted = false;
    char prev_password[MAX_PASSWORD_LENGTH] = {};

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
    bool subscription_request = false;

    while (1) {

        //Generates new password
        if (first || password_decrypted) {
            password_decrypted = false;
            memcpy(prev_password, password, password_length);
            generate_printable_password(password, password_length);
            MTA_get_rand_data(key, password_length / 8);
            encrypt_password(password, key, encrypted, password_length, password_length / 8);
            print_new_pw(password, key, encrypted);
            if (!first) {
                for (int i = 0; i < decrypters_count; i++) {
                    send_msg_to_decryptor_pipe(decrypters_pipes[i], encrypted, password_length);
                }
            }
            first = false;
        }

        if (timeout_seconds > 0) {
            bool good_guess = false;
            time_t start_time = time(NULL);
            while (time(NULL) - start_time < timeout_seconds){
                if(!password_decrypted)
                    read_encrypter_pipe_data(pipe_fd, &decrypters_count, &current_guess, encrypted);
                if (current_guess.is_pending) {
                    good_guess = check_guess(&current_guess, password, &password_decrypted, prev_password);
                    if (good_guess)
                        break;
                }
            }
            if (!good_guess)
                print_timeout();
            password_decrypted = true;
        }
        else {
            if(!password_decrypted)
                read_encrypter_pipe_data(pipe_fd, &decrypters_count, &current_guess, encrypted);
            if (current_guess.is_pending) {
                bool isCorrect = check_guess(&current_guess, password, &password_decrypted, prev_password);
            }
        }
    }
    close(pipe_fd);
    return NULL;
}