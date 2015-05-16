/*
 * Crypter header file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#define SECRET BASE_DIR "/secret.pem"
#define PASS_SIZE 100

int encrypt_data(char *pwd_to_encrypt);
int check_hash(const unsigned char *hash_to_check);
void encrypt_root();