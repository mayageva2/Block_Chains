#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <ctype.h>
#include <pthread.h>
#include <mta_crypt.h>
#include <mta_rand.h>
#include "decrypt.h"        

//This function checks if all characters in buffer are printable
bool is_printable(char* buf, int len) 
{
    for (int i = 0; i < len; i++) 
        if (!isprint(buf[i])) return false;
    
    return true;
}

//This function attempts to decrypt the password
bool decryption_attempt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, char* guess) 
{
    char decrypted[PASSWORD_DECRYPTER_LENGTH] = {0};
    unsigned int out_len = enc_len;

    MTA_CRYPT_RET_STATUS status = MTA_decrypt(key, key_len, encrypted, enc_len, decrypted, &out_len);

    if (status != MTA_CRYPT_RET_OK || !is_printable(decrypted, out_len))
        return false; //Attempt failed

    memcpy(guess, decrypted, out_len);
    return true;
}

void printSendLog(Decrypter* args, unsigned int password_length, char* guess, char* key_used, int it) {

  printf("%lu\t[DECRYPTER #%d]\t[INFO]\tAfter decryption(%.*s), key guessed(%.*s), sending to encrypter after %d iterations\n",
    pthread_self(),
    args->id,
    password_length, guess,
    password_length/8, key_used,
    it);
}

void* decryptProcess(void* argument)
{
    Decrypter* args = (Decrypter*) argument;
    SharedData* shared = args->shared;
    int key_len = password_length/8; //Global argument: "password_length"
    char key[LAST_KEY_LENGTH] = {};
    int iter = 0; //Counter for number iterations in brute-force loop
    char guess[MAX_PASSWORD_LENGTH] = {};

    while(running) //Continue as long as encryptor sending passwords
    {
      pthread_mutex_lock(&shared->mutex); 

      while (!shared->new_data)
          pthread_cond_wait(&shared->cond, &shared->mutex);

      if (!running) {
          pthread_mutex_unlock(&shared->mutex);
          break;
      }

      if (shared->decrypted) {
          pthread_mutex_unlock(&shared->mutex);
          continue;
      }

      char encrypted_local [MAX_PASSWORD_LENGTH] = {};
      memcpy(encrypted_local, shared->encrypted, password_length);//Copy encrypted password for decryptor use
      pthread_mutex_unlock(&shared->mutex); 

      iter = 0; 
      
      while(running)//Brute-force loop
      { 

        iter++;
        MTA_get_rand_data(key,key_len);

        //Try to decrypt password
        bool ok = decryption_attempt(encrypted_local, password_length, key, key_len, guess);
        if (!ok)
            continue;

        //From now on, decryption attempt can be made
        pthread_mutex_lock(&shared->guess_mutex);
        if (shared->guess_pending) { //Case: encrypter is busy with another decrypter
          pthread_mutex_unlock(&shared->guess_mutex);
          continue;
        }

        memcpy(shared->guess, guess, password_length);
        shared->guesser_id = args->id;
        shared->guess_pending = true;
        printSendLog(args, password_length, guess, key, iter); //Prints the send log of the decrypter
        pthread_cond_signal(&shared->guess_cond);

        while (shared->guess_pending) //Case: Encrypter is still working with my guess
            pthread_cond_wait(&shared->guess_cond, &shared->guess_mutex);

        if (shared->guess_result) {
            pthread_mutex_unlock(&shared->guess_mutex);
            break;
        }   
      }
    }
    free(args);
    return NULL;
}

pthread_t* create_decrypter_threads(int num,SharedData* shared,DecryptionResult* res)
{
    pthread_t* threads = malloc(num*sizeof(pthread_t)); //Array of threads IDs
    if(!threads){
        printf("Failed allocating memory for threads\n");
        exit(1);
    }

    for(int i=0;i<num;i++) //Create num decrypters
    {
      Decrypter* args = malloc(sizeof(Decrypter)); //Decrypters arguments
      if(!args){
        printf("Failed allocating memory for decryptor\n");
        exit(1);
      }

      //Set arguments
      args->id = i;
      args->shared = shared;
      args->res = res;

      //Create thread and run decryptProcess func with args
      if(pthread_create(&threads[i], NULL, decryptProcess, args) != 0){
        printf("Failed creating pthread\n");
        free(args);
        exit(1);
      }
    }
    return threads; //Return array of threads IDs
}