#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
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
#include <stdbool.h>
#include "decrypt_funcs.h"
#include "config.h"

#define ENCRYPTER_PIPE_FILE_PATH "/mnt/mta/encrypter_pipe"
#define CONFIG_FILE_PATH "/mnt/mta/mtacrypt.conf"
#define MAX_PASSWORD_LENGTH 1024
#define MAX_PIPE_NAME_LENGTH 128
#define MAX_MSG_LEN (MAX_PIPE_NAME_LENGTH + 1 + MAX_PASSWORD_LENGTH)


//Global variables
FILE* log_fp = NULL;
int decrypter_id = -1;


int main () {
    bool running=true;
    bool newPwd = false;
    
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
    MTA_crypt_init();

    //Register with the encrypter
   int reg_fd = open(ENCRYPTER_PIPE_FILE_PATH, O_WRONLY);
    if (reg_fd < 0 ) {
        log_message("ERROR", "Error: failed to open encrypter pipe - %s", strerror(errno));
        unlink(pipe_name);
		close_file_logging();
        exit(1);
    }

    char reg_msg[MAX_PIPE_NAME_LENGTH + 16] = {};
    strncpy(reg_msg, pipe_name, sizeof(reg_msg) - 1);
    if (write(reg_fd, reg_msg, strlen(reg_msg)) < 0) {
        log_message("ERROR", "Error: %s", strerror(errno));
        close(reg_fd);
        unlink(pipe_name);
		close_file_logging();
        exit(1);
    }
    close(reg_fd);
    log_message("INFO", "Sent connect request to server");

    //Open our Named Pipe for blocking read
    int fd = open(pipe_name, O_RDWR);
    if (fd < 0) {
        log_message("ERROR", "Error: %s", strerror(errno));
        unlink(pipe_name);
		close_file_logging();
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
        log_message("ERROR", "Error: failed to read first password - %s", strerror(errno));
		unlink(pipe_name);
		close_file_logging();
        exit(1);
    }
    else if (n == 0) {
        log_message("ERROR", "Error: Pipe closed unexpectedly");
		unlink(pipe_name);
		close_file_logging();
        exit(1);
    }
    log_message("INFO", "Received encrypted password");
    newPwd=true;

    //Switch to non-blocking mode fd
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		log_message("ERROR", "Failed to set non-blocking reading from fd");
		unlink(pipe_name);
		close_file_logging();
		exit(1);
	}

    int iter = 0;
    //Brute-force loop
    while (running) {

       iter++;

        //Each iteration check blocking for a new password pushed by encrypter
        ssize_t m = read(fd, encrypted, enc_len);
        if (m == enc_len){ //Case: received new encrypted password
            log_message("INFO", "Received new encrypted password %s", encrypted);
            newPwd = true;
        }
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
        if (!try_decrypt(encrypted, enc_len, key, key_len, guess)){ //Case: is not a viable guess
            continue;
        }

        //Send our guess back; format: "<pipe_name> <guess>"
        if(newPwd) {
            newPwd = false;
            char msg[MAX_MSG_LEN] = {};
            int msglen = snprintf(msg, sizeof(msg), "%s ", pipe_name);
            memcpy(msg+msglen, guess, enc_len);
            msglen += enc_len;

            int gfd = open(ENCRYPTER_PIPE_FILE_PATH, O_WRONLY); //Guess File Descriptor
            if (gfd < 0) {
                log_message("ERROR", "Error: %s", strerror(errno));
                break;
            }
            if (write(gfd, msg, msglen) < 0) {
                log_message("ERROR", "Error: %s", strerror(errno));
                close(gfd);
                break;
            }
            close(gfd);
            log_message("INFO", "Decrypted password: %s, key: %s (in %d iterations)", guess, key, iter);
        }
    }

    //Cleanup
    close(fd);
    unlink(pipe_name);
    close_file_logging();
    return 0;
    
}