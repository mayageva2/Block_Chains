#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <string.h> 
#include <ctype.h>
#include <mta_crypt.h>
#include <mta_rand.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include "decrypt.h"

#define ENCRYPTER_PIPE_FILE_PATH "/mnt/mta/encrypter_pipe"
#define CONFIG_FILE_PATH "/mnt/mta/mtacrypt.conf"
#define MAX_PASSWORD_LENGTH 1024
#define MAX_PIPE_NAME_LENGTH 128
#define MAX_MSG_LEN (MAX_PIPE_NAME_LENGTH + 1 + MAX_PASSWORD_LENGTH)

//Global variables
FILE* log_fp = NULL;
int decrypter_id = -1;

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

void log_message(const char* level, const char* fmt, ...) {
    if (!log_fp) //Case: log file is not valid
        return;

    time_t now = time(NULL);
    fprintf(log_fp, "%ld\t[DECRYPTER #%d]\t[%s]\t", now, decrypter_id, level);

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

void close_file_logging() {
    if (log_fp) { //Case: file is still open
        fclose(log_fp);
        log_fp = NULL;
    }
}

//This function reads the password length from a config file
int read_config_password_length(const char* path) {

    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Open config file failed");
        exit(1);
    }

    int len = 0;
    if (fscanf(file, "PASSWORD_LENGTH=%d", &len) != 1) {
        fprintf(stderr, "Invalid config file formatin %s", path);
        fclose(file);
        exit(1);
    }
    fclose(file);

    if (len <= 0 || len > MAX_PASSWORD_LENGTH || (len%8) != 0) {
        fprintf(stderr,
            "Config error: password length must be greater than 0, smaller than %d and divisible by 8 (got %d)\n",
            MAX_PASSWORD_LENGTH, len);
            exit(1);
    }

    return len;
}

int main () {

    bool running = true;

    //Read password length
    int password_length = read_config_password_length(CONFIG_FILE_PATH);
    unsigned int enc_len = password_length;
    unsigned int key_len = password_length / 8;

    //Determine our unique decrypter ID by finding next vacant number
    int id = 1;
    char pipe_name[MAX_PIPE_NAME_LENGTH] = {};
    while (running) {
        snprintf(pipe_name, sizeof(pipe_name), "/mnt/mta/decrypter_pipe_%d", id);
        if (mkfifo(pipe_name, 0666) == 0) //Case: a new named pipe was created successfully
            break;
        else if (errno == EEXIST) { //Case: id already taken, try next
            id++;
            continue;
        }
        perror("mkfifo decrypter pipe failed");
        exit(1);
    }
    init_file_logging(id);

    //Register with the encrypter
    int reg_fd = open(ENCRYPTER_PIPE_FILE_PATH, O_WRONLY);
    if (reg_fd < 0) {
        log_message("ERROR", "Error: %s", sterror(errno));
        unlink(pipe_name);
        exit(1);
    }
    char reg_msg[MAX_PIPE_NAME_LENGTH + 16] = {};
    snprintf(reg_msg, sizeof(reg_msg), "REGISTER:%s", pipe_name);
    if (write(reg_fd, reg_msg, strlen(reg_msg)) < 0) {
        log_message("ERROR", "Error: %s", sterror(errno));
        close(reg_fd);
        unlink(pipe_name);
        exit(1);
    }
    close(reg_fd);
    log_message("INFO", "Sent connect request to server");

    //Open our Named Pipe for blocking read
    int fd = open(pipe_name, O_RDONLY);
    if (fd < 0) {
        log_message("ERROR", "Error: %s", sterror(errno));
        unlink(pipe_name);
        exit(1);
    }

    //Buffers for encryption/decryption
    char encrypted[MAX_PASSWORD_LENGTH] = {};
    char key[MAX_PASSWORD_LENGTH/8] = {};
    char guess[MAX_PASSWORD_LENGTH] = {};
    char answer[32] = {};

    //Wait (blocking) for the first encrypted password
    ssize_t n = read(fd, encrypted, enc_len);
    if (n < 0) {
        log_message("ERROR", "Error: failed to read first password - %s", sterror(errno));
        exit(1);
    }
    else if (n == 0) {
        log_message("ERROR", "Error: Pipe closed unexpectedly");
        exit(1);
    }
    log_message("INFO", "Received encrypted password");

    //Switch to non-blocking mode fd
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int iter = 1;
    //Brute-force loop
    while (running) {
        //Each iteration check blocking for a new password pushed by encrypter
        ssize_t m = read(fd, encrypted, enc_len);
        if (m == enc_len) //Case: received new encrypted password
            log_message("INFO", "Received new encrypted password %s", encrypted);
        else if (m == -1 && errno == EAGAIN) {
            //No new password
        }
        else if (m == 0) {
            log_message("ERROR", "Pipe was closed by encrypter. Exiting.");
            break;
        }
        else
            log_message("ERROR", "Partial password or unexpected read: m = %zd", m);

        //Generate a random key and try decrypt
        MTA_get_rand_data(key, key_len);
        if (!try_decrypt(encrypted, enc_len, key, key_len, guess)) //Case: is not a viable guess
            continue;

        //Send our guess back; format: "<pipe_name> <guess>"
        char msg[MAX_MSG_LEN] = {};
        int msglen = snprintf(msg, sizeof(msg), "%s ", pipe_name);
        memcpy(msg+msglen, guess, enc_len);
        msglen += enc_len;

        int gfd = open(ENCRYPTER_PIPE_FILE_PATH, O_WRONLY); //Guess File Descriptor
        if (gfd < 0) {
            log_message("ERROR", "Error: %s", sterror(errno));
            break;
        }
        if (write(gfd, msg, msglen) < 0) {
            log_message("ERROR", "Error: %s", sterror(errno));
            close(gfd);
            break;
        }
        close(gfd);

        //Wait for answer on our pipe (non blocking)
        ssize_t a;
        do {
            a = read(fd, answer, sizeof(answer)-1);
            if (a < 0 && errno == EAGAIN) {
                usleep(10 * 1000); //10ms
                continue;
            }
            if (a <= 0) {
                log_message("ERROR", "Error: %s", sterror(errno));
                continue;
            }
        } while (a <= 0);

        answer[a] = '\0';
        if (strcmp(answer, "OK") == 0) //Case: guess was correct!
            log_message("INFO", "Decrypted password: %s, key: %s (in %d iterations)", guess, key, iter);

        iter++;
    }

    //Cleanup
    close(fd);
    unlink(pipe_name);
    close_file_logging();
    return 0;
}