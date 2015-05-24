/*-------------------------------------------------------------------------*/
/**
   \file	ui.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	User interface file

   Contains all routines to execute actions linked to args in cli
*/
/*--------------------------------------------------------------------------*/

#include <global.h>
#include <core/ui.h>
#include <quarantine/quarantine.h>
#include <core/scanner.h>
#include <net/network.h>
#include <net/crypter.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <logging/logging.h>


int net_exiter()
{
	int len, s_cl;
	int f_tok;
	char *query, *res;
	struct sockaddr_in remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("[UI] Unable to create socket");
		return -1;
	}

	bzero((char *) &remote, sizeof(remote));
	remote.sin_family = AF_INET;
	inet_aton("127.0.0.1", &remote.sin_addr.s_addr);
	remote.sin_port = htons(SOCK_NET_PORT);

	if (connect(s_cl, (struct sockaddr *)&remote, sizeof(remote)) == -1) {
		perror("[UI] Unable to connect the qr_sock");
		goto error;
	}
	
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if ((res == NULL)) {
		perror("[UI] Unable to allocate memory");
		goto error;
	}

	f_tok = open(TOKEN_DB, O_RDONLY);
	char inner_token[255];
	if (read(f_tok, inner_token, 255) < 0) {
		perror("[UI] Unable to read the inner token");
		close(f_tok);
		return -1;
	}
	close(f_tok);

	if (write(s_cl, inner_token, 255) < 0) {
		perror("[UI] Unable to send token");
		goto error;
	}

	if (read(s_cl, res, 1) < 0) {
		perror("[UI] Unable to get ack");
		goto error;
	}

	FILE *f = fopen(SECRET, "rb");
	if (!f) {
		perror("[UI] Unable to open secret file");
		goto error;
	}
	unsigned char digest[SHA256_DIGEST_LENGTH];
	if (fread(digest, SHA256_DIGEST_LENGTH, 1, f) < 0) {
		perror("[UI] Unable to read digest in secret file");
		fclose(f);
		goto error;
	}
	fclose(f);


	if (write(s_cl, digest, SHA256_DIGEST_LENGTH) < 0) {
		perror("[UI] Unable to send encrypted password");
		goto error;
	}

	if (read(s_cl, res, 1) < 0) {
		perror("[UI] Unable to get ack");
		goto error;
	}

	snprintf(query, len, "%d:0", KL_EXIT);

	if (write(s_cl, query, len) < 0) {
		perror("[UI] Unable to send query");
		goto error;
	}

	if (read(s_cl, res, 1) < 0) {
		perror("[UI] Unable to get query result");
		goto error;
	}

	if (read(s_cl, res, 1) < 0) {
		perror("[UI] Unable to get query result");
		goto error;
	}


	close(s_cl);
	free(res);
	free(query);
	return 0;

error:
	close(s_cl);
	free(res);
	free(query);
	return -1;
}


void execute_commands(int nb, char **commands)
{
	if (!strcmp(commands[1], "-add-to-qr")) {
		int i;
		struct stat new_s;
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel -add-to-qr <filename>\n");
			exit(EXIT_FAILURE);
		} else {
			for (i = 2; i < nb; i++) {
				if (stat(commands[i], &new_s) < 0) {
					printf("File %s could not be found\n", commands[i]);
					return;
				}
			}
		}
		qr_query(commands, QR_ADD);
	} else if (!strcmp(commands[1], "-rm-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				 "Correct syntax is: klearnel -rm-fom-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(commands, QR_RM);
	} else if (!strcmp(commands[1], "-rm-all-from-qr")) {
		qr_query(commands, QR_RM_ALL);
	} else if (!strcmp(commands[1], "-get-qr-list")) {
		qr_query(commands, QR_LIST);
	} else if (!strcmp(commands[1], "-get-qr-info")) {
		if (nb < 3) {
			fprintf(stderr, "Element to detail missing\n"
				 "Correct syntax is: klearnel -get-qr-info <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(commands, QR_INFO);
	} else if (!strcmp(commands[1], "-restore-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to restore missing\n"
				 "Correct syntax is: klearnel -restore-from-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(commands, QR_REST);
	} else if (!strcmp(commands[1], "-restore-all-from-qr")) {
		qr_query(commands, QR_REST_ALL);
	} else if (!strcmp(commands[1], "-view-rt-log")) {
		char date[7];
		time_t rawtime;
	  	struct tm * timeinfo;
	  	time(&rawtime);
	  	timeinfo = localtime(&rawtime);
	  	strftime(date, sizeof(date), "%y%m%d", timeinfo);
	  	char *logs = malloc(strlen(LOG_DIR) + strlen(date) + strlen(".log") + 1);
	  	char *task = malloc(strlen(logs)+20);

	  	if (!logs || !task) {
	  		perror("UI: Unable to allocate memory");
	  		exit(EXIT_FAILURE);
	  	}
	  	if (snprintf(logs, strlen(LOG_DIR) + strlen(date) + strlen(".log") + 1, "%s%s%s", LOG_DIR, date,".log") < 0){
	  		perror("LOG: Unable print path for logs");
			exit(EXIT_FAILURE);
	  	} 
	  	if (access(logs, F_OK) == -1) {
	  		perror("UI: Unable to find log file");
	  		exit(EXIT_FAILURE);
	  	}

	  	sprintf(task, "tail -f %s", logs);
	  	system(task);
	} else if (!strcmp(commands[1], "-add-to-scan")) {
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel -add-to-scan <file/folder path>\n");
			exit(EXIT_FAILURE);
		}
		scan_query(nb, commands, SCAN_ADD);
	} else if (!strcmp(commands[1], "-rm-from-scan")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				"Correct syntax is: klearnel -rm-from-scan <file/folder path to remove>\n");
			exit(EXIT_FAILURE);
		}
		scan_query(nb, commands, SCAN_RM);
	} else if (!strcmp(commands[1], "-get-scan-list")) {
		scan_query(nb, commands, SCAN_LIST);
	} else if (!strcmp(commands[1], "-license")) {
		NOT_YET_IMP;
		printf("See the LICENSE file located in /etc/klearnel\n");
	} else if (!strcmp(commands[1], "-stop")) {
		printf("Stopping Klearnel services\n\n");
		if (net_exiter() != 0) {
			printf("Check Klearnel logs, Networker did not terminate correctly\n");
		} else {
			printf("Networker process successfully stopped\n");
		}
		if (qr_query(commands, KL_EXIT) != 0) {
			printf("Check Klearnel logs, Qr-Worker did not terminate correctly\n");
		} else {
			printf("Qr-Worker successfully stopped\n");
		}
		if (scan_query(nb, commands, KL_EXIT) != 0) {
			printf("Check Klearnel logs, Scanner did not terminate correctly\n");
		} else {
			printf("Scanner process successfully stopped\n");
		}
		printf("\nKlearnel services are stopped and the module will now be shutted down\n");
	} else if (!strcmp(commands[1], "-help")) {
		printf("\n\e[4mKlearnel commands:\e[24m\n\n");
		printf(" - \e[1m-add-to-qr <path-to-file>\e[21m:\n\t Add a new file to the quarantine\n");
		printf(" - \e[1m-rm-from-qr <filename>\e[21m:\n\t Remove the specified file from the quarantine"
			"\n\tWARNING: this command will definitively remove the file!\n");
		printf(" - \e[1m-rm-all-from-qr\e[21m:\n\t Remove all files from the quarantine"
			"\n\tWARNING: this command will definitively remove the files!\n");
		printf(" - \e[1m-get-qr-list\e[21m:\n\t Display the file currently stored in quarantine\n");
		printf(" - \e[1m-get-qr-info <filename>\e[21m:\n\t NOT YET IMPLEMENTED\n");
		printf(" - \e[1m-restore-from-qr <filename>\e[21m:\n\t Restore the specified file to its original location\n");
		printf(" - \e[1m-restore-all-from-qr\e[21m:\n\t Restore all files to their original locations\n");
		printf(" - \e[1m-add-to-scan <file/folder path>\e[21m:\n\t Add the specified file or folder to the scanner watch list"
			"\n\tNOTE: this command will prompt you for each action to apply to the new item. It can take a few minutes to complete.\n");
		printf(" - \e[1m-rm-from-scan <file/folder path>\e[21m: \n\t Remove the specified file/folder from the scanner watch list\n");	
		printf(" - \e[1m-view-rt-log\e[21m:\n\t Display the current klearnel's log in real time\n");
		printf(" - \e[1m-license\e[21m:\n\t Display the klearnel license terms\n");
		printf(" - \e[1m-start\e[21m:\n\t Start Klearnel service\n");
		printf(" - \e[1m-stop\e[21m:\n\t Stop Klearnel service\n");
		printf(" - \e[1m-help\e[21m:\n\t Display this help message\n");
		printf("\nCopyright (C) 2014, 2015 Klearnel-Devs\n\n");
	} else {
		fprintf(stderr, "Unknown command\n"
			"Try \"klearnel -help\" to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
