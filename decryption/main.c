#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <mta_crypt.h> 
#include "shared.h"
#include "encrypt.h"
#include "decrypt.h"
#include "config.h"

//Global variables
SharedData shared = {
    .length = 0,
    .new_data = false,
    .decrypted = false,
    .guess_pending = false,
    .guesser_id = -1,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .guess_cond = PTHREAD_COND_INITIALIZER 
};
int password_length;
int num_decrypters;
int timeout_seconds = 0;
bool running = true;

//This function prints the correct usage of the program when incorrect or missing arguments are provided
void print_usage() {
    printf("Usage: encrypt.out [-t|--timeout seconds] <-n|--num-of-decrypters <number>> <-l|--password-length <length>>\n");
}

//Main
int main(int argc, char *argv[]) 
{
    //For using MTA decrypt and encrypt
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize MTA crypto library\n");
        return 1;
    }

    //Read password_length from config file
    password_length = read_config_password_length("../mtacrypt.conf");

    static struct option long_options[] = {
        {"num-of-decrypters", required_argument, 0, 'n'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    int got_n = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "n:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                num_decrypters = atoi(optarg);
                got_n = 1;
                break;
            case 't':
                timeout_seconds = atoi(optarg);
                break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

   if (!got_n) {
       fprintf(stderr, "Missing num of decrypters.\n");
       print_usage();
       exit(EXIT_FAILURE);
   } 

   if (password_length <= 0 || password_length % 8 != 0 || password_length > MAX_PASSWORD_LENGTH) {
       fprintf(stderr, "Password length must be > 0, divisible by 8, and <= %d.\n", MAX_PASSWORD_LENGTH);
       exit(EXIT_FAILURE);
   }

   //Decrypter threads creation
   pthread_t* decrypter_threads = create_decrypter_threads(num_decrypters); //Call decryptors

   pthread_t encrypter_thread;
   if (pthread_create(&encrypter_thread, NULL, encrypter, NULL) != 0) //Added memory allocation check
   {
       printf("Failed allocate memory for encryptor thread");
       exit(1);
   }

   pthread_join(encrypter_thread, NULL);

   //Wait for decryptors work
   pthread_mutex_lock(&shared.mutex);
   pthread_cond_broadcast(&shared.cond);
   pthread_cond_broadcast(&shared.guess_cond);
   pthread_mutex_unlock(&shared.mutex);

   for (int i = 0; i < num_decrypters; i++) 
       pthread_join(decrypter_threads[i], NULL);

   free(decrypter_threads);
   return 0;
}