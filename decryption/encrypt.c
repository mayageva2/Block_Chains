#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <mta_crypt.h> 
#include <mta_rand.h>
#include "encrypt.h"
#include "decrypt.h"

SharedData shared = {
    .length = 0,
    .new_data = false, 
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .decrypted = false
};

//Global arguments
int password_length;
int num_decrypters;
int timeout_seconds = 0;
bool running = true;

//Helper to generate a printable random password
void generate_printable_password(char *password, int length) {
    for (int i=0; i< length; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint(c));
        password[i] = c;
    }
}

//Encrypt password with MTA encrypt
void encrypt_password(char *password,char *key, char *encrypted, int length, int key_length) 
{
    unsigned int out_len = length;
    MTA_CRYPT_RET_STATUS status = MTA_encrypt(key,key_length, password, length, encrypted, &out_len);
}

//Print Encrypter new password generat information
void printEncrypterInfo(char password[], char key [], char encrypted []) {

    pthread_t tid = pthread_self();
    printf("%lu \t", tid); //Prints encrypter's thread id
    printf("[Encrypter] \t [INFO]   New password generated: %.*s, ", password_length, password); //Prints the password decrypted
    printf("key: %.*s, ", password_length/8, key); //Prints the key used to encrypt
    printf("After encryption: %.*s", password_length, encrypted); //Prints the encrypted password
    printf("\n");
}

//Encrypter thread function
void *encrypter(void *arg) {
    DecryptionResult* res = (DecryptionResult*) arg;
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];
    pthread_t tid = pthread_self();

    while (running) {
        //Reset new_data flag before creating a new password
        pthread_mutex_lock(&shared.mutex);
        shared.new_data = false;
        pthread_mutex_unlock(&shared.mutex);

        generate_printable_password(password, password_length);
        MTA_get_rand_data(key, password_length / 8);
        encrypt_password(password, key, encrypted, password_length, password_length / 8);

        pthread_mutex_lock(&shared.mutex);
        printEncrypterInfo(password, key, encrypted);
        pthread_mutex_unlock(&shared.mutex);

        //Write encrypted password to shared buffer
        pthread_mutex_lock(&shared.mutex);
        memcpy(shared.original_pass, password, password_length);
        memcpy(shared.encrypted, encrypted, password_length);
        shared.length = password_length;
        shared.new_data = true; //New encrypted password available
        shared.decrypted = false; //Encrypted password wasn't decrypted
        pthread_cond_broadcast(&shared.cond);
        pthread_mutex_unlock(&shared.mutex);

        printf("%lu \t", tid); //Prints encrypter's thread id
        printf("[Encrypter] \t [INFO]   Generated new password and key. Waiting for decrypters...\n");

        //Wait for timeout or correct decryption
       time_t start = time(NULL);
       while (true) {
            sleep(1); //Simulate wait

            pthread_mutex_lock(&shared.mutex);
            char received[MAX_PASSWORD_LENGTH];
            memcpy(received, res->password, password_length);
            received[password_length] = '\0';
            int dec_id = res->decrypter_id;
            bool done = shared.decrypted;
            pthread_mutex_unlock(&shared.mutex);

            if (done) {
                printf("%lu \t", tid); //Prints encrypter's thread id      
                printf("[Encrypter] \t [OK]     Password was successfully decrypted, by [Decrypter #%d].", dec_id);
                printf("Recieved: %s, Is: %s\n", received,password);
                break;
            }

            if (timeout_seconds > 0 && time(NULL) - start >= timeout_seconds) {
                printf("%lu \t", tid); //Prints encrypter's thread id 
                printf("[Encrypter] \t [ERROR]  Timeout expired(%d seconds). Generating new password.\n", timeout_seconds);
                break;
            }
       
        }

    }

    return NULL;
}