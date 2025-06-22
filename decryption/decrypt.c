#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <string.h> 
#include <ctype.h>
#include <pthread.h>
#include <mta_crypt.h>
#include <mta_rand.h>
#include "shared.h"
#include "decrypt.h"

//Global variables
extern SharedData shared;
extern int password_length;
extern bool running;

//This function returns true if the guess is printable and the decryption was successful and false otherwise
bool try_decrypt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, char* guess) {

    unsigned int out_len = enc_len;
    if (MTA_decrypt(key, key_len, encrypted, enc_len, guess, &out_len) != MTA_CRYPT_RET_OK)
        return false;

    for (unsigned int i = 0; i < out_len; i++)
        if (!isprint(guess[i]))
            return false;

    return true;
}

//This function prints a log when a decrypter sends a password guess to the encrypter
void print_send_log(int id, char* guess, char* key_used, int it) {

    printf("%lu\t[DECRYPTER #%d]\t[INFO]\tAfter decryption(%.*s), key guessed(%.*s), sending to encrypter after %d iterations\n",
    pthread_self(),
    id,
    password_length, guess,
    password_length / 8, key_used,
    it);
}

//This function is executed by each decrypter thread and performs brute-force decryption attempts
void* decryptProcess(void* arg)
{
    int id = (int)(intptr_t)arg;
    char encrypted_local [MAX_PASSWORD_LENGTH] = {};
    char key[MAX_PASSWORD_LENGTH/8] = {};
    char guess[MAX_PASSWORD_LENGTH] = {};

    while(running) //Continue as long as encryptor sending passwords
    {
        pthread_mutex_lock(&shared.mutex);

        while (!shared.new_data && running)
            pthread_cond_wait(&shared.cond, &shared.mutex);

        if (!running) {
            pthread_mutex_unlock(&shared.mutex);
            break;
        }

        memcpy(encrypted_local, shared.encrypted, shared.length);
        shared.new_data = false;
        pthread_mutex_unlock(&shared.mutex);

        for (int iter = 1; running; iter++) //Brute-force loop; counter for number iterations in brute-force loop
        {
            MTA_get_rand_data(key, password_length / 8);
            if (!try_decrypt(encrypted_local, password_length, key, password_length / 8, guess))
                continue;

            //From now on, decryption attempt can be made
            //Send candidate guess
            pthread_mutex_lock(&shared.guess_mutex);
            while (shared.guess_pending) //Case: encrypter is busy with another decrypter
                pthread_cond_wait(&shared.guess_cond, &shared.guess_mutex);

            shared.guess_pending = true;
            shared.guesser_id = id;
            memcpy(shared.guess, guess, password_length);
            pthread_cond_signal(&shared.guess_cond);
            pthread_mutex_unlock(&shared.guess_mutex);

            print_send_log(id, guess, key, iter); //Prints the send log of the decrypter
            break;
        }
    }
    return NULL;
}

//This function creates the specified number of decrypter threads and returns an array of their pthread IDs
pthread_t* create_decrypter_threads(int num)
{
    pthread_t* threads = malloc(num * sizeof(pthread_t)); //Array of threads IDs
    if (!threads) {
        printf("Failed allocating memory for threads\n");
        exit(1);
    }

    for (int i = 0; i < num; i++) //Create num decrypters
    {
        //Create thread and run decryptProcess func with args
        if (pthread_create(&threads[i], NULL, decryptProcess, (void*)(intptr_t)i) != 0) {
            printf("Failed creating pthread\n");
            exit(1);
        }
    }

    return threads; //Return array of threads IDs
}