#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include "mta_rand.h"
#include <mta_crypt.h> 

#define MAX_PASSWORD_LENGTH 256

typedef struct {
    char encrypted[MAX_PASSWORD_LENGTH]; // encrypted password
    char original_pass [MAX_PASSWORD_LENGTH]; //original password
    int length;
    int new_data; //new_data=1 means new encrypted password available
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool decrypted; //success flag, decrypted=1 means that the password was decrypted
} SharedData;

SharedData shared = {
    .length = 0,
    .new_data = 0, 
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .decrypted = false //CHANGED THIS
};

//Global arguments-??
int password_length = 32;
int num_decrypters = 1;
int timeout_seconds = 0;
bool running = true; // ADDED THIS

// Helper to generate a printable random password
void generate_printable_password(char *password, int length) {
    for (int i = 0; i < length; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint(c));
        password[i] = c;
    }
}

// Encrypt password with MTA encrypt -I DON'T KNOW WHAT IS THE STATUS BUT BESIDES THIS, ITS OK
void encrypt_password(char *password,char *key, char *encrypted, int length, int key_length) 
{
    unsigned int out_len = length;

    MTA_CRYPT_RET_STATUS status = MTA_encrypt(key,key_length, password, length, encrypted, &out_len);

}

// Encrypter thread function
void *encrypter(void *arg) {
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];

    while (running) {
        generate_printable_password(password, password_length);
        MTA_get_rand_data(key, password_length / 8);
        encrypt_password(password, key, encrypted, password_length, password_length / 8);

        //GAL - SHOULD I STEAL THE SHARED DATA AND PRINT THE PASSWORD AND KEY LIKE GABI DID, HERE?
        //MAYBE ALSO MENTION THE NUMBER OF THE THREAD?

        // Write encrypted password to shared buffer
        pthread_mutex_lock(&shared.mutex);
        memcpy(shared.original_pass, password, password_length);
        memcpy(shared.encrypted, encrypted, password_length);
        shared.length = password_length;
        shared.new_data = 1; //new encrypted password available
        shared.decrypted = false; //encrypted password wasn't decrypted
        pthread_cond_broadcast(&shared.cond);
        pthread_mutex_unlock(&shared.mutex);

        printf("[Encrypter] Generated new password and key. Waiting for decrypters...\n");

        // Wait for timeout or correct decryption - IMPLEMENTED BY GAL
       time_t start = time(NULL);
       while (true) {
            sleep(1); // simulate wait

            pthread_mutex_lock(&shared.mutex);
            bool done = shared.decrypted;
            pthread_mutex_unlock(&shared.mutex);

            if (done) {

                printf("[Encrypter] Password was successfully decrypted.\n");
                break;
            }

            if (timeout_seconds > 0 && time(NULL) - start >= timeout_seconds) {
                printf("[Encrypter] Timeout expired. Generating new password.\n");
                break;
            }
       
        }

    }

    return NULL;
}
