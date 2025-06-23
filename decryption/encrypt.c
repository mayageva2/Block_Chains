#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <mta_crypt.h> 
#include <mta_rand.h>
#include "shared.h"
#include "encrypt.h"

//Global variables
extern SharedData shared;
extern int password_length;
extern int num_decrypters;
extern int timeout_seconds;
extern bool running;

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
void print_new_pw(pthread_t tid, char* password, char* key, char* encrypted) {

    printf("%lu\t[ENCRYPTER]\t[INFO]\tNew password generated: %.*s, key: %.*s, After encryption: %.*s\n",
    tid,
    password_length, password,
    password_length/8, key,
    password_length, encrypted);
}

//This function prints a success log when a correct password guess is received from a decrypter
void print_success(pthread_t tid, char* password) {

    printf("%lu\t[ENCRYPTER]\t[OK]\tPassword decrypted successfully by client #%d, received (%.*s), is (%.*s)\n",
    tid,
    shared.guesser_id,
    password_length, shared.guess,
    password_length, password);
}

//This function prints an error log when no password guess is received within the configured timeout
void print_timeout(pthread_t tid) {

    printf("%lu\t[ENCRYPTER]\t[ERROR]\tNo password received during configured timeout period (%d seconds), regenerating password\n",
    tid,
    timeout_seconds);
}

//This function prints a log when a decrypter submits a correct but outdated password
void print_old_pw_guess(pthread_t tid, int decrypter_id, char* guess) {

    printf("%lu\t[ENCRYPTER]\t[ERROR]\tReceived correct but outdated password from client #%d: (%.*s)\n",
    tid,
    decrypter_id,
    password_length, guess);
}

//This function prints an error log when a wrong password guess is submitted by a decrypter
void print_wrong_guess(pthread_t tid, int decrypter_id, char* guess, char* password) {

    printf("%lu\t[ENCRYPTER]\t[ERROR]\tWrong password received from client #%d (%.*s), should be (%.*s)\n",
    tid,
    decrypter_id,
    password_length, guess,
    password_length, password);
}

//This function is executed by the encrypter thread, and coordinates password generation and validation; Encrypter thread function
void *encrypter(void *arg) {
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];
    pthread_t tid = pthread_self();

    while (running) {
        if (shared.decrypted || shared.length == 0) { 
            shared.decrypted = false;
            memcpy(shared.previous_password, password, password_length);
            generate_printable_password(password, password_length);
            MTA_get_rand_data(key, password_length / 8);
            encrypt_password(password, key, encrypted, password_length, password_length / 8);

            //Write encrypted password to shared buffer
            pthread_mutex_lock(&shared.mutex);
            memcpy(shared.encrypted, encrypted, password_length);
            shared.length = password_length;
            shared.new_data = true; //New encrypted password available
            shared.decrypted = false; //Encrypted password wasn't decrypted
            shared.guess_pending = false;
            pthread_cond_broadcast(&shared.cond);
            pthread_mutex_unlock(&shared.mutex);

            print_new_pw(tid, password, key, encrypted); //Prints new password info
        }

        

        if (timeout_seconds > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_seconds;

            pthread_mutex_lock(&shared.guess_mutex);
            if (!shared.decrypted) {
                int rc = pthread_cond_timedwait(&shared.guess_cond, &shared.guess_mutex, &ts);

                if (rc == ETIMEDOUT) { //Case: timeout
                    print_timeout(tid);
                    shared.decrypted = true; //Simulate new password
                    pthread_mutex_unlock(&shared.guess_mutex);
                    continue;
                }
            }
            pthread_mutex_unlock(&shared.guess_mutex);
        }
        else { //Case: no -t flag was given
            pthread_mutex_lock(&shared.guess_mutex);
            if (!shared.decrypted)
                pthread_cond_wait(&shared.guess_cond, &shared.guess_mutex);
            pthread_mutex_unlock(&shared.guess_mutex);
        }


        //Mark the slot free for the next guess
        shared.guess_pending = false;
        int decrypter_id = shared.guesser_id;
        char guess_curr[MAX_PASSWORD_LENGTH] = {};
        memcpy(guess_curr, shared.guess, password_length);
        pthread_mutex_unlock(&shared.guess_mutex);

        bool match = (memcmp(guess_curr, password, password_length) == 0);
            
            if (match) {
                pthread_mutex_lock(&shared.mutex);
                if (!shared.decrypted) {
                    shared.decrypted = true;
                    print_success(tid, password);
                    pthread_cond_broadcast(&shared.cond);
                }
                else
                    print_old_pw_guess(tid, decrypter_id, guess_curr); //Prints log of old password guess
                
                pthread_mutex_unlock(&shared.mutex);
            }
            else {
                bool old_match = (memcmp(guess_curr, shared.previous_password, password_length) == 0);
                if (old_match) //Case: Old password guess
                    print_old_pw_guess(tid, decrypter_id, guess_curr);
                else //Case: Wrong guess
                    print_wrong_guess(tid, decrypter_id, guess_curr, password);
            }
    }
    return NULL;
}