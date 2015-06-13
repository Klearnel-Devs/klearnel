/**
 * \file crypter.h
 * \brief Crypter header 
 * 
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 * 
 * This file contains all structure, constants and protytpes needed by 
 * the crypter routines
 */

#define SECRET BASE_DIR "/secret.pem"
#define PASS_SIZE 100

bool simpleSHA256(void* input, unsigned long length, unsigned char* md);
/*-------------------------------------------------------------------------*/
/**
 \brief Encrypt pwd_to_encrypt with SHA256 and store it in SECRET file
 \param pwd_to_encrypt the password to encrypt
 \return Returns 0 on success and -1 on error
*/
/*-------------------------------------------------------------------------*/
int encrypt_data(char *pwd_to_encrypt);
/*-------------------------------------------------------------------------*/
/**
 \brief Check if hash_to_check corresponds to the encrypted password stored in SECRET
 \param hash_to_check the hash to verify
 \return Returns:
	- -1 on error
	- 0 if the passwords are the same
	- 1 if the passwords are not the same
*/
/*-------------------------------------------------------------------------*/
int check_hash(const unsigned char *hash_to_check);
/*-------------------------------------------------------------------------*/
/**
 \brief At first klearnel (or if the SECRET file is deleted), ask for root password to ecnrypt it 
 \return void
*/
/*-------------------------------------------------------------------------*/
void encrypt_root();