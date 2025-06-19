#pragma once

#include "shared.h"

#define PASSWORD_DECRYPTER_LENGTH MAX_PASSWORD_LENGTH
#define LAST_KEY_LENGTH PASSWORD_DECRYPTER_LENGTH/8 

typedef struct {
    char password[PASSWORD_DECRYPTER_LENGTH]; //Password after decypher
    char last_key[LAST_KEY_LENGTH];           //Last key checked
    pthread_mutex_t lock;                     //Read and write lock
    pthread_cond_t cond;                      //New attempt signal
} DecryptionResult;

typedef struct{
    int id;                      //Decrypter id
    SharedData* shared;          //Pointer to shared data with encryptor
    DecryptionResult* res;       //Pointer to personal result 
}Decrypter;

//Decrypter thread functions - any other function in the c file is a helper to these functions.
//Therefore, we don't want to give access to these functions from outside the c file.

//Decrypter thread function
void* decryptProcess(void* argument);

//Create and return an array of decrypter threads
pthread_t* create_decrypter_threads(int num, SharedData* shared, DecryptionResult* res);