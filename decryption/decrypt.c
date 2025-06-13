#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <mta_rand.h>
#include "encryptor.c"

typedef struct {
    char password[128];          // password after decypher
    char last_key[16];           // last key checked
    pthread_mutex_t lock;        // read and write lock
    pthread_cond_t cond;         // new attempt signal
} DecryptionResult;

typedef struct{
    int id;                      //decrypter id
    SharedData* shared;          //pointer to shared data with encryptor
    DecryptionResult* res;    //pointer to personal result 
}Decrypter;            

//this func checks if the password is valid
int is_valid(char* guess,char* original_password, int len) 
{
    return (memcmp(guess, original_password, len) == 0);
}

// this func checks if all characters in buffer are printable
int is_printable(char* buf, int len) 
{
    for (int i = 0; i < len; i++) 
    {
        if (!isprint(buf[i])) return 0;
    }
    return 1;
}

// this func attempts to decrypt the password                                                 //maybe change shared to diffrent name? like result
int decryption_attempt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, DecryptionResult* shared) 
{
    char decrypted[128] = {0};
    unsigned int out_len = enc_len;

    MTA_CRYPT_RET_STATUS status = MTA_decrypt(key, key_len, encrypted, enc_len, decrypted, &out_len);

    if (status != MTA_CRYPT_RET_OK || !is_printable(decrypted, out_len)) {
  return 0; //attempt failed
}
        
    pthread_mutex_lock(&shared->lock);

    memcpy(shared->password, decrypted, out_len);
    memcpy(shared->last_key, key, key_len);

    pthread_cond_signal(&shared->cond);
    pthread_mutex_unlock(&shared->lock);
    printf("[DEBUG] Decrypted: %.*s\n", out_len, decrypted);
    return 1;
}

int continue_decryption(SharedData* shared,int* new_data)
{
   pthread_mutex_lock(&shared->mutex);

   int pass_changed=(shared->new_data==0);
   int was_solved=shared->decrypted;

   pthread_mutex_unlock(&shard->mutex);

   if(pass_changed || was_solved){return 0;} //will stop descryption

   return 1; //continue descryption
}


void* decryptProcess(void* argument)
{
    Decrypter* args=(Decrypter*) argument;
    SharedData* shared=args->shared;
    DecryptionResult* res=args->res;
    int key_length=password_length/8; //global argument: "password_length"
    char encrypted_local[128];
    char key[16];
    int iter=0; //counter for number iterations in brute-force loop
    int new_data_avail=0;

    while(running) //continue as long as encryptor sending passwords
    {
      pthread_mutex_lock(&shared->mutex); 
      while(!shared->new_data && running){ //sleep until encryptor provides new password
        pthread_cond_wait(&shared->cond,&shared->mutex);
      }

      memcpy(encrypted_local,shared->encrypted,password_length);//  copy encrypted password for decryptor use
      pthread_mutex_unlock(&shared->mutex); 
      iter=0; 
      
      while(running)//brute-force loop
      { 
         MTA_get_rand_data(key,key_length);

         //try to decrypt password
         int success= decryption_attempt(encrypted_local,password_length,key,key_length,res);
        if(success) //if decryption attempt succeeded
        {
            pthread_mutex_lock(&res->lock); 
            int valid=is_valid(res->password,shared->original,password_length); //check if decrypted password matches the real one
            if(valid)
            {
              printf("After decryption %s, key guessed %s, sending to server after %d iterations\n",
              res->password,res->last_key,iter);

              pthread_mutex_lock(&shared->mutex);
              shared->decrypted=1; //mark password as decrypted
              pthread_cond_signal(&shared->cond); //signal encryptor to proceed
              pthread_mutex_unlock(&shared->mutex);
              pthread_mutex_unlock(&res->lock);

              break; //stop brute force
            }
            //incorrect password
             pthread_mutex_unlock(&res->lock);
        }
        //check if new password was encrypted
        if(!continue_decryption(shared,&new_data_avail)){break;}
        iter++;
      }
    }
    free(args);
    return NULL;
}

pthread_t* create_decrypter_threads(int num,SharedData* shared,DecryptionResult* res)
{
    pthread_t* threads=malloc(num*sizeof(pthread_t)); //array of threads IDs
    if(!threads){
        printf("Failed allocate memory for threads");
        exit(1);
    }

    for(int i=0;i<num;i++) //create num decryptors
    {
      Decrypter* args=malloc(sizeof(Decrypter)); //Decryptors arguments
      if(!args){
        printf("Failed allocate memory for decryptor");
        exit(1);
      }
      //set arguments
      args->id=i;
      args->shared=shared;
      args->res=res;

      //create thread and run decryptProcess func with args
      if(pthread_create(&threads[i],NULL,decryptProcess,args)!=0){
        printf("Failed create pthread");
        free(args);
        exit(1);
      }
    }
    return threads; //return array of threads IDs
}
