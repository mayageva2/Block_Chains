#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include "encrypt.h"
#include "decrypt.h"

//Main
int main(int argc, char *argv[]) 
{
    //For using MTA decrypt and encrypt
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize MTA crypto library\n");
        return 1;
    }


    static struct option long_options[] = {
        {"num-of-decrypters", required_argument, 0, 'n'},
        {"password-length", required_argument, 0, 'l'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "n:l:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                num_decrypters = atoi(optarg);
                break;
            case 'l':
                password_length = atoi(optarg);
                break;
            case 't':
                timeout_seconds = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -n <num-decrypters> -l <password-length> [-t <timeout>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (password_length <= 0 || password_length % 8 != 0 || password_length > MAX_PASSWORD_LENGTH) {
        fprintf(stderr, "Password length must be > 0, divisible by 8, and <= %d.\n", MAX_PASSWORD_LENGTH);
        exit(EXIT_FAILURE);
    }

    pthread_t encrypter_thread;
    if(pthread_create(&encrypter_thread, NULL, encrypter, NULL)!=0) //Added memory allocation check
    {
      printf("Failed allocate memory for encryptor thread");
      exit(1);
    }

    //Decrypter threads creation
   DecryptionResult res_shared={0};
   pthread_mutex_init(&res_shared.lock,NULL);
   pthread_cond_init(&res_shared.cond,NULL);
   pthread_t* decrypter_threads = create_decrypter_threads(num_decrypters,&shared,&res_shared); //Call decryptors


    pthread_join(encrypter_thread, NULL);
    //Wait for decryptors work
    running=false;
    pthread_mutex_lock(&shared.mutex);
    pthread_cond_broadcast(&shared.cond);
    pthread_mutex_unlock(&shared.mutex);

    for(int i=0;i<num_decrypters;i++){
        pthread_join(decrypter_threads[i],NULL);
    }

    free(decrypter_threads); //ADDED THIS - DELETE WHEN YOU READ IT
    
    return 0; 
}
