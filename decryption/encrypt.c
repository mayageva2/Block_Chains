#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <mta_crypt.h>
#include "config.h"
#include "encrypt_funcs.h"

#define MAX_PASSWORD_LENGTH 1024
#define MAX_INFO_NAME_LENGTH 128

int password_length;
int num_decrypters;
int timeout_seconds;

void print_usage() {
    printf("Usage: encrypt.out [-t|--timeout seconds]\n");
}

int main(int argc, char *argv[]) {
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize MTA crypto library\n");
        return 1;
    }

    password_length = read_config_password_length("/mnt/mta/mtacrypt.conf");

    static struct option long_options[] = {
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 't':
                timeout_seconds = atoi(optarg);
                break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    if (password_length <= 0 || password_length % 8 != 0 || password_length > MAX_PASSWORD_LENGTH) {
        fprintf(stderr, "Password length must be > 0, divisible by 8, and <= %d.\n", MAX_PASSWORD_LENGTH);
        exit(EXIT_FAILURE);
    }

    encrypter(NULL);
    return 0;
}
