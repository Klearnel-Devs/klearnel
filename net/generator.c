/**
 * \file generator.c
 * \brief Token generator
 * 
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 *
 * This file contains all functions needed to generate
 * the token used to identified klearnel module on the network
 */
#include <global.h>
#include <math.h>
#include <net/network.h>

int generate_token() 
{
	if (access(TOKEN_DB, F_OK) != -1) {
		return 0;
	}
	time_t t_stamp = time(NULL);
	srand((unsigned)t_stamp);
	int ratio = rand() % 100;
	int num_host = 0;
	int i;
	char *host = malloc(sizeof(char)*256);

	gethostname(host, (sizeof(char)*256));
	
	for (i = 0; i < strlen(host); i++) {
		num_host += (int)host[i];
	}
	unsigned long num_tk = (t_stamp * sqrt(ratio)) * ((num_host != 0) ? num_host : 1);
	char token[255];
	sprintf(token, "KL%lu", num_tk);

	printf("Please specify the path where to store the generated token\n"
	       "(this is only to be done at first launch): ");
	char path[PATH_MAX];
	scanf("%s", path);
	char *path_user = malloc(sizeof(char)*(PATH_MAX+256));
	sprintf(path_user, "%s/%s_token.txt", path, host); 
	int oldmask = umask(0);
	FILE *f_user = fopen(path_user, "w");
	if (!f_user) {
		printf("\nUnable to open a file in the specified path.\nPlease check that the path is correct.\n\n");
		free(host);
		free(path_user);
		return -1;
	}

	fprintf(f_user, "%s", token);
	fclose(f_user);

	int f_db = open(TOKEN_DB, O_CREAT | O_WRONLY | O_TRUNC, USER_RW);
	write(f_db,  token, (sizeof(char)*255));
	close(f_db);
	umask(oldmask);

	printf("The token has been stored to %s successfully.\n"
	       "Please save it in a safe place!\n"
	       "This token is used to manage this device from klearnel-manager\n", path_user);
	printf("\nPress any key to continue...");
	getchar();
	getchar();
	free(path_user);
	free(host);
	return 0;
}