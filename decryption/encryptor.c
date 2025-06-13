#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int decrypted; //success flag, decrypted=1 means that the password was decrypted
} SharedData;

SharedData shared = {
    .length = 0,
    .new_data = 0, 
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

//Global arguments-??
int password_length = 32;
int num_decrypters = 1;
int timeout_seconds = 0;

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

/*/ Encrypt password with simple XOR cipher using a repeating key
void encrypt_password(const char *password, const char *key, char *encrypted, int length, int key_length) {
    for (int i = 0; i < length; ++i) {
        encrypted[i] = password[i] ^ key[i % key_length];
    }
}/*/

// Encrypt password with MTA encrypt -GAL CHECK IT
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

        // Write encrypted password to shared buffer
        pthread_mutex_lock(&shared.mutex);
        memcpy(shared.original_pass, password, password_length);
        memcpy(shared.encrypted, encrypted, password_length);
        shared.length = password_length;
        shared.new_data = 1; //new encrypted password available
        shared.decrypted = 0; //encrypted password wasn't decrypted
        pthread_cond_broadcast(&shared.cond);
        pthread_mutex_unlock(&shared.mutex);

        printf("[Encrypter] Generated new password and key. Waiting for decrypters...\n");

        // Wait for timeout or correct decryption (not implemented yet)
       time_t start = time(NULL);
       while (1) {
            sleep(1); // simulate wait
            if (timeout_seconds > 0 && time(NULL) - start >= timeout_seconds) {
                printf("[Encrypter] Timeout expired. Generating new password.\n");
                break;
            }
            
            //Placeholder for checking descrypted input...- decryptor checks on his own correct password?
            // If correct decrypted password received:
            // break;
       
        }

    }

    return NULL;
}
