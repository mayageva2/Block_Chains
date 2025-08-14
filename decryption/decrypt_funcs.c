#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <mta_crypt.h>
#include <mta_rand.h>
#include "decrypt_funcs.h"

//Global variables
extern int decrypter_id;
extern FILE* log_fp;

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

//This function initializes file logging for the given decrypter ID
void init_file_logging(int id) {
    decrypter_id = id;

    char log_path[64] = {};
    snprintf(log_path, sizeof(log_path), "/var/log/decrypter_%d.log", id);

    log_fp = fopen(log_path, "a");
    if (!log_fp) { //Case: failed to open the log file
        perror("Failed to open log file");
        exit(1);
    }
}

//This function logs a message at the specified level to both the opened log file and stdout
void log_message(const char* level, const char* fmt, ...) {
    if (!log_fp) //Case: log file is not valid
        return;

    time_t now = time(NULL);

    //Prepare message body
    char message [1024] = {};
    va_list args;
    va_start(args,fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    //Write to log file
    fprintf(log_fp, "%ld\t[DECRYPTER #%d]\t[%s]\t%s\n", now, decrypter_id, level, message);
    fflush(log_fp);

    //Also print to stdout
    printf("%ld\t[DECRYPTER #%d]\t[%s]\t%s\n", now, decrypter_id, level, message);
    fflush(stdout);
}

//This function closes the log file if it is open and does nothing otherwise
void close_file_logging() {
    if (log_fp) { //Case: file is still open
        fclose(log_fp);
        log_fp = NULL;
    }
}
