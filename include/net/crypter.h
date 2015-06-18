/**
 * \file crypter.h
 * \brief Crypter header 
 * 
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 * 
 * This file contains all structure, constants and protytpes needed by 
 * the crypter routines
 */
#ifndef _KLEARNEL_CRYPTER_H
#define _KLEARNEL_CRYPTER_H
/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/
/**
  \brief The secret file in Klearnel base folder
 */
#define SECRET BASE_DIR "/secret.pem"
/**
  \brief Define the size of a password
 */
#define PASS_SIZE 100
/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
 \brief Create a SHA256 encrypted value from input into md
 \param input the value to hash
 \param length the length of the value to hash
 \param md the variable in wich the hash generated will be stored
 \return Returns true if the hash has been successfully generated else returns false
*/
/*-------------------------------------------------------------------------*/
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

#endif
