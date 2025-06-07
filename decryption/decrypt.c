#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <mta_crypt.h>
#include <mta_rand.h>

char current_password[16];  //current password

typedef struct {
    char password[128];          // password after decypher
    char last_key[16];           // last key chaecked
    int ready;                   // new attempt flag
    pthread_mutex_t lock;        // read and write lock
    pthread_cond_t cond;         // new attempt signal
} DecryptionResult;

//this func checks if the password is valid
int is_valid(char* guess, int len) 
{
    return (memcmp(guess, current_password, len) == 0);
}

// this func checks if all characters in buffer are printable
int is_printable(char* buf, int len) 
{
    for (int i = 0; i < len; i++) 
    {
        if (!isprint(buf[i])) return 0;
    }
    return 1;
}

// this func attempts to decrypt the password
int decryption_attempt(char* encrypted, unsigned int enc_len, char* key, unsigned int key_len, DecryptionResult* shared) 
{
    char decrypted[128] = {0};
    unsigned int out_len = enc_len;

    MTA_CRYPT_RET_STATUS status = MTA_decrypt(key, key_len, encrypted, enc_len, decrypted, &out_len);

    if (status != MTA_CRYPT_RET_OK || !is_printable(decrypted, out_len)) 
        return 0; //attemppt failed
    

    pthread_mutex_lock(&shared->lock);

    memcpy(shared->password, decrypted, out_len);
    memcpy(shared->last_key, key, key_len);
    shared->ready = 1;

    pthread_cond_signal(&shared->cond);
    pthread_mutex_unlock(&shared->lock);

    return 1;
}


