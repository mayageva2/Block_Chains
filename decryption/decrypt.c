#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <ctype.h>
#include <pthread.h>
#include <mta_crypt.h>
#include <mta_rand.h>
#include "decrypt.h"        

//This function checks if the password is valid
bool is_valid(char* guess,char* original_password, int len)
{
    return (memcmp(guess, original_password, len) == 0);
}

//This function checks if all characters in buffer are printable
bool is_printable(char* buf, int len) 
{
    for (int i = 0; i < len; i++) 
    {
        if (!isprint(buf[i])) return false;
    }
    return true;
}

//This function attempts to decrypt the password                                                 //changed shared to res (just seems more acurate)
bool decryption_attempt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, DecryptionResult* res) 
{
    char decrypted[PASSWORD_DECRYPTER_LENGTH] = {0};
    unsigned int out_len = enc_len;

    MTA_CRYPT_RET_STATUS status = MTA_decrypt(key, key_len, encrypted, enc_len, decrypted, &out_len);

    if (status != MTA_CRYPT_RET_OK || !is_printable(decrypted, out_len)) {
    return false; //Attempt failed
  }
        
    pthread_mutex_lock(&res->lock);
    memcpy(res->password, decrypted, out_len);
    memcpy(res->last_key, key, key_len);
    pthread_cond_signal(&res->cond);
    pthread_mutex_unlock(&res->lock);
    return true;
}

bool continue_decryption(SharedData* shared)
{
   pthread_mutex_lock(&shared->mutex);
   bool was_solved = shared->decrypted; //Password was decrypted
   pthread_mutex_unlock(&shared->mutex); 

   return !was_solved; //Continue descryption
}

void* decryptProcess(void* argument)
{
    Decrypter* args = (Decrypter*) argument;
    SharedData* shared = args->shared;
    DecryptionResult* res = args->res;
    int key_length=password_length/8; //Global argument: "password_length"
    char encrypted_local[PASSWORD_DECRYPTER_LENGTH] = {};
    char key[LAST_KEY_LENGTH] = {};
    int iter = 0; //Counter for number iterations in brute-force loop

    while(running) //Continue as long as encryptor sending passwords
    {
      pthread_mutex_lock(&shared->mutex); 
      while(!shared->new_data && running){ //Wait until encryptor provides new password
        pthread_cond_wait(&shared->cond,&shared->mutex);
      }

      memcpy(encrypted_local,shared->encrypted,password_length);//Copy encrypted password for decryptor use
      
      pthread_mutex_unlock(&shared->mutex); 
      iter=0; 
      
      while(running)//Brute-force loop
      { 
        //Check if new password was encrypted
        if(!continue_decryption(shared)){break;}

        iter++;
         MTA_get_rand_data(key,key_length);

         //Try to decrypt password
         bool success = decryption_attempt(encrypted_local,password_length,key,key_length,res);
        if(success) //If decryption attempt succeeded
        {
            pthread_mutex_lock(&res->lock); 
            if(shared->decrypted){ //If password was decrypted do nothing
              pthread_mutex_unlock(&res->lock);
              continue;
            }

            int valid = is_valid(res->password,shared->original_pass,password_length); //Check if decrypted password matches the real one
         
            if(valid)
            {
              printf("[Decrypter #%d]  [INFO]  ",args->id);
              printf("After decryption %s, key guessed %s, sending to server after %d iterations\n",
              res->password,res->last_key,iter);

              pthread_mutex_lock(&shared->mutex);
              shared->decrypted = true; //Mark password as decrypted
              shared->new_data = false;
              pthread_cond_signal(&shared->cond); //Signal encryptor to proceed
              pthread_mutex_unlock(&shared->mutex);
              pthread_mutex_unlock(&res->lock);

              break; //Stop brute force
            }
            //Incorrect password
             pthread_mutex_unlock(&res->lock);
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
        printf("Failed allocating memory for threads");
        exit(1);
    }

    for(int i=0;i<num;i++) //Create num decrypters
    {
      Decrypter* args = malloc(sizeof(Decrypter)); //Decrypters arguments
      if(!args){
        printf("Failed allocating memory for decryptor");
        exit(1);
      }
      //Set arguments
      args->id=i;
      args->shared=shared;
      args->res=res;

      //Create thread and run decryptProcess func with args
      if(pthread_create(&threads[i],NULL,decryptProcess,args) != 0){
        printf("Failed creating pthread");
        free(args);
        exit(1);
      }
    }
    return threads; //Return array of threads IDs
}