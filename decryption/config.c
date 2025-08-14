#include <stdio.h>
#include <stdlib.h>
#include "config.h"

//This func reads length of password
int read_config_password_length(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        perror("open config");
        exit(1);
    }

    int len = 0;
    fscanf(f, "password_length=%d", &len);
    fclose(f);

    if (len <= 0 || len > MAX_PASSWORD_LENGTH || len % 8 != 0) {
        fprintf(stderr, "Invalid password length in config.\n");
        exit(1);
    }

    return len;
}
