#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include "mta_rand.h"

#define MAX_PASSWORD_LENGTH 256

typedef struct {
    char encrypted[MAX_PASSWORD_LENGTH];
    int length;
    int new_data;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SharedData;

SharedData shared = {
    .length = 0,
    .new_data = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

int password_length = 32;
int num_decrypters = 1;
int timeout_seconds = 0;

int running = 1;

// Helper to generate a printable random password
void generate_printable_password(char *password, int length) {
    for (int i = 0; i < length; ++i) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint(c));
        password[i] = c;
    }
}

// Encrypt password with simple XOR cipher using a repeating key
void encrypt_password(const char *password, const char *key, char *encrypted, int length, int key_length) {
    for (int i = 0; i < length; ++i) {
        encrypted[i] = password[i] ^ key[i % key_length];
    }
}

// Encrypter thread function
void *encrypter(void *arg) {
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_PASSWORD_LENGTH / 8];
    char encrypted[MAX_PASSWORD_LENGTH];

    while (running) {
        generate_printable_password(password, password_length);
        MTA_get_rand_data(key, password_length / 8);
        encrypt_password(password, key, encrypted, password_length, password_length / 8);

        // Write encrypted password to shared buffer
        pthread_mutex_lock(&shared.mutex);
        memcpy(shared.encrypted, encrypted, password_length);
        shared.length = password_length;
        shared.new_data = 1;
        pthread_cond_broadcast(&shared.cond);
        pthread_mutex_unlock(&shared.mutex);

        printf("[Encrypter] Generated new password and key. Waiting for decrypters...\n");

        // Wait for timeout or correct decryption (not implemented yet)
        time_t start = time(NULL);
        while (1) {
            sleep(1); // simulate wait
            if (timeout_seconds > 0 && time(NULL) - start >= timeout_seconds) {
                printf("[Encrypter] Timeout expired. Generating new password.\n");
                break;
            }
            // Placeholder for checking decrypted input...
            // If correct decrypted password received:
            // break;
        }
    }

    return NULL;
}

// Main
int main(int argc, char *argv[]) {
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
    pthread_create(&encrypter_thread, NULL, encrypter, NULL);

    // (Decrypter threads would be created here...)

    pthread_join(encrypter_thread, NULL);
    return 0;
}