#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <mta_crypt.h>
#include <mta_rand.h>

//Functions Declarations
bool try_decrypt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, char* guess);
void init_file_logging(int id);
void log_message(const char* level, const char* fmt, ...);
void close_file_logging();
int read_config_password_length(const char* path);

#define MAX_PASSWORD_LENGTH 1024