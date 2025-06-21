#pragma once
#include <pthread.h>

//Decrypter thread functions - any other function in the c file is a helper to these functions.
//Therefore, we don't want to give access to these functions from outside the c file.

//Decrypter thread function
void* decryptProcess(void* argument);

//Create and return an array of decrypter threads
pthread_t* create_decrypter_threads(int num);