/**
 * \file crypter.c
 * \brief SHA-256 password encryptor
 * 
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <openssl/sha.h>
#include <logging/logging.h>
#include <global.h>
#include <termios.h>
#include <net/crypter.h>

bool simpleSHA256(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if(!SHA256_Init(&context))
        return false;

    if(!SHA256_Update(&context, (unsigned char*)input, length))
        return false;

    if(!SHA256_Final(md, &context))
        return false;

    return true;
}

int encrypt_data(char *pwd_to_encrypt)
{
	unsigned char md[SHA256_DIGEST_LENGTH];
	if (simpleSHA256(pwd_to_encrypt, strlen(pwd_to_encrypt), md)) {
		LOG(INFO, "Password encrypted successfully");
		FILE *f = fopen(SECRET, "wb");
		if (fwrite(md, 1, SHA256_DIGEST_LENGTH, f) < 0) {
			LOG(WARNING, "Unable to store encrypted password in secret.pem");
			fclose(f);
			return -1;
		}
		fclose(f);
	}
	return 0;
}

int check_hash(const unsigned char *hash_to_check)
{
	FILE *f = fopen(SECRET, "rb");
	if (!f) {
		LOG(WARNING, "Unable to open secret file");
		return -1;
	}
	unsigned char digest[SHA256_DIGEST_LENGTH];
	if (fread(digest, SHA256_DIGEST_LENGTH, 1, f) < 0) {
		LOG(WARNING, "Unable to read digest in secret file");
		fclose(f);
		return -1;
	}
	fclose(f);
    int i;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		if (digest[i] != hash_to_check[i])
			return 1;
	}
	return 0;
}

void getPassword(char password[])
{
    static struct termios oldt, newt;
    int i = 0;
    int c;

    /*saving the old settings of STDIN_FILENO and copy settings for resetting*/
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;

    /*setting the approriate bit in the termios struct*/
    newt.c_lflag &= ~(ECHO);          

    /*setting the new bits*/
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    /*reading the password from the console*/
    while ((c = getchar())!= '\n' && c != EOF && i < PASS_SIZE){
        password[i++] = c;
    }
    password[i] = '\0';

    /*resetting our old STDIN_FILENO*/ 
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

}

void encrypt_root() 
{
	if (access(SECRET, F_OK) != -1) 
		return;
	char password[PASS_SIZE];
	printf("Please enter a password: ");
    	getPassword(password);
    	printf("\n");

    	encrypt_data(password);

    	printf("Please enter the password again: ");
    	char pass_check[PASS_SIZE];
    	getPassword(pass_check);
    	printf("\n");

    	unsigned char md[SHA256_DIGEST_LENGTH];
    	simpleSHA256(pass_check, strlen(pass_check), md);
    	int result = check_hash(md);

    	if (result < 0) {
    		printf("Error unable to check passwords\n");
    	} else if (result == 1) {
    		printf("Passwords don't seem to correspond\n");
            unlink(SECRET);
            printf("Press any key to continue...");
            getchar();
            system("clear");
            encrypt_root();
    	} else {
    		printf("Passwords are the same!\n"
    		       "You will not be asked to enter it anymore\n"
    		       "NOTE: If you want to change the password, delete the file "
    		       "/etc/klearnel/secret.pem and restart klearnel\n");
    	}
}