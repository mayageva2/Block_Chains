#pragma once

#include <pthread.h>
#include <stdbool.h>

#define MAX_PASSWORD_LENGTH 1024

typedef struct {
    char encrypted[MAX_PASSWORD_LENGTH]; //Encrypted password
    char guess [MAX_PASSWORD_LENGTH]; //Guessed password by a decrypter
    int length;

    bool new_data; //True means new encrypted password available
    bool guess_pending;
    bool guess_result;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_mutex_t guess_mutex;
    pthread_cond_t guess_cond;

    bool decrypted; //Success flag, true means that the password was decrypted
    int guesser_id;
} SharedData;

//Global variables
extern SharedData shared;
extern int password_length;
extern int num_decrypters;
extern int timeout_seconds;
extern bool running;