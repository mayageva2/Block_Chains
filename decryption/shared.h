#pragma once

#include <pthread.h>
#include <stdbool.h>

#define MAX_PASSWORD_LENGTH 1024

typedef struct {
    char encrypted[MAX_PASSWORD_LENGTH]; // encrypted password
    char original_pass [MAX_PASSWORD_LENGTH]; //original password
    int length;
    bool new_data; //true means new encrypted password available
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool decrypted; //success flag, true means that the password was decrypted
} SharedData;

//Global variables
extern SharedData shared;
extern int password_length;
extern int num_decrypters;
extern int timeout_seconds;
extern bool running;