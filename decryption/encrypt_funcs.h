#pragma once

//Global variables
extern int password_length;
extern int num_decrypters;
extern int timeout_seconds;

//Encrypter thread function - any other function in the c file is a helper to this function.
//Therefore, we don't want to give access to these functions from outside the c file
void* encrypter(void* arg);