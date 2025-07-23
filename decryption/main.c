#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <mta_crypt.h> 
#include "shared.h"
#include "encrypt_funcs.h"
#include "config.h"

//Global variables
SharedData shared = {
    .length = 0,
    .new_data = false,
    .decrypted = false,
    .guess_pending = false,
    .guesser_id = -1,
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

   return 0;
}