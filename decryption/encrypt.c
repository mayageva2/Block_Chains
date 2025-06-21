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
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .guess_mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .guess_cond = PTHREAD_COND_INITIALIZER,
    .new_data = false,
    .guess_pending = false,
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
    MTA_encrypt(key,key_length, password, length, encrypted, &out_len);
}

//Prints a log when Encrypter generates new password information
void printEncrypterNewPassword(pthread_t tid, char* password, char* key, char* encrypted) {

    printf("%lu\t[ENCRYPTER]\t[INFO]\tNew password generated: %.*s, key: %.*s, After encryption: %.*s\n",
    tid,
    password_length, password,
    password_length/8, key,
    password_length, encrypted);
}

//Prints a log when the password was decrypted
void printSuccessfulDecryption(pthread_t tid, char* password) {

    printf("%lu\t[ENCRYPTER]\t[OK]\tPassword decrypted successfully by client #%d, received (%.*s), is (%.*s)\n",
    tid,
    shared.guesser_id,
    password_length, shared.guess,
    password_length, password);
}

void printTimeout(pthread_t tid, int timeout_seconds) {

    printf("%lu\t[ENCRYPTER]\t[ERROR]\tNo password received during configured timeout period (%d seconds), regenerating password\n",
    tid,
    timeout_seconds);
}

//Encrypter thread function
void *encrypter(void *arg) {
    DecryptionResult* res = (DecryptionResult*) arg;
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];
    pthread_t tid = pthread_self();

    while (running) {

        pthread_mutex_lock(&shared.mutex);

        generate_printable_password(password, password_length);
        MTA_get_rand_data(key, password_length / 8);
        encrypt_password(password, key, encrypted, password_length, password_length / 8);

        printEncrypterNewPassword(tid, password, key, encrypted); //Prints new password info 

        //Write encrypted password to shared buffer
        memcpy(shared.encrypted, encrypted, password_length);
        shared.length = password_length;
        shared.new_data = true; //New encrypted password available
        shared.decrypted = false; //Encrypted password wasn't decrypted
        shared.guess_pending = false;
        shared.guess_result = false;
        pthread_cond_broadcast(&shared.cond);
        pthread_mutex_unlock(&shared.mutex);

        //Wait for timeout or correct decryption
       time_t start = time(NULL);
       while (running && !shared.decrypted) {
            pthread_mutex_lock(&shared.guess_mutex);
            while (!shared.guess_pending)
                pthread_cond_wait(&shared.guess_cond, &shared.guess_mutex);

            bool match = (memcmp(shared.guess, password, password_length) == 0);
            shared.guess_result = match;
            shared.guess_pending = false;
            pthread_cond_broadcast(&shared.guess_cond);
            pthread_mutex_unlock(&shared.guess_mutex);

            if (match) {
                pthread_mutex_lock(&shared.mutex);
                shared.decrypted = true;
                shared.new_data = false;
                printSuccessfulDecryption(tid, password);
                pthread_cond_broadcast(&shared.cond);
                pthread_mutex_unlock(&shared.mutex);

                pthread_mutex_lock(&shared.guess_mutex);
                shared.guess_pending = false; //Let decrypters proceed
                pthread_cond_broadcast(&shared.guess_cond);
                pthread_mutex_unlock(&shared.guess_mutex);
                break;
            }

            if (timeout_seconds > 0 && time(NULL) - start >= timeout_seconds) {
                printTimeout(tid, timeout_seconds);

                //Must reset shared state and notify decrypters
                pthread_mutex_lock(&shared.guess_mutex);
                shared.guess_pending = false;
                pthread_cond_broadcast(&shared.guess_cond);
                pthread_mutex_unlock(&shared.guess_mutex);
                break;
            }      
        }
    }
    return NULL;
}