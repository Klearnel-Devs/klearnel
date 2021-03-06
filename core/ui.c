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

/*-------------------------------------------------------------------------*/
/**
  \brief  Send the command KL_EXIT to the network process
  \return Return 0 on success, else -1
  
 */
/*--------------------------------------------------------------------------*/
int _net_exiter()
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
	inet_aton("127.0.0.1", &remote.sin_addr);
	remote.sin_port = htons(SOCK_NET_PORT);

	if (connect(s_cl, (struct sockaddr *)&remote, sizeof(remote)) == -1) {
		printf("Network service is not running\n");
		return -2;
	}
	
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if (res == NULL) {
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
	if ((int)fread(digest, SHA256_DIGEST_LENGTH, 1, f) < 0) {
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
		perror("[UI] Unable to get command result");
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

/*-------------------------------------------------------------------------*/
/**
 \brief Remove the mutex in case of crash
 \return Returns 0 on success, else -1

*/
/*-------------------------------------------------------------------------*/
int _kill_mutex()
{
	key_t mutex_key = ftok(IPC_RAND, IPC_MUTEX);
	int mutex = semget(mutex_key, 1, IPC_CREAT | IPC_EXCL | IPC_PERMS);

	if (semctl(mutex, 0, IPC_RMID, NULL) < 0) {
		return -1;
	}
	return 0;
}

int stop_action() {
	printf("Stopping Klearnel services\n\n");
	int net_res = _net_exiter();
	if (net_res == -1) {
		printf("Check Klearnel logs, Networker did not terminate correctly\n");
	} else if (net_res == 0) {
		printf("Networker process successfully stopped\n");
	}

	int qr_res = qr_query(NULL, KL_EXIT);
	if (qr_res == -1) {
		printf("Check Klearnel logs, Quarantine did not terminate correctly\n");
	} else if (qr_res == 0) {
		printf("Quarantine successfully stopped\n");
	}

	int scan_res = scan_query(NULL, KL_EXIT);
	if (scan_res == -1) {
		printf("Check Klearnel logs, Scanner did not terminate correctly\n");
	} else if (scan_res == 0) {
		printf("Scanner process successfully stopped\n");
	}
	
	if ((scan_res == 0) && (qr_res == 0) && (net_res == 0)) {
		printf("\nKlearnel services are stopped and the module will now be shutted down\n");
		return 0;
	} else if ((scan_res == -2) && (qr_res == -2) && (net_res == -2)) {
		printf("\nKlearnel services are currently not running\n");
		return 0;
	} else {
		printf("\nAt least on service is not running\n"
			"It is often mean that a service has crashed\n"
			"To check it, run the following command as root: ps aux | grep klearnel\n"
			"If should show 3 processes. If one of it is marked <defunct> or is missing,\n"
			"Run the command \"pkill klearnel\" as root and restart it\n"
			"If any process is running, you can restart Klearnel safely\n");
		return -1;
	}	
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
		scan_query(commands, SCAN_ADD);
	} else if (!strcmp(commands[1], "-rm-from-scan")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				"Correct syntax is: klearnel -rm-from-scan <file/folder path to remove>\n");
			exit(EXIT_FAILURE);
		}
		scan_query(commands, SCAN_RM);
	} else if (!strcmp(commands[1], "-get-scan-list")) {
		scan_query(commands, SCAN_LIST);
	} else if (!strcmp(commands[1], "-force-scan")) {
		scan_query(commands, SCAN_FORCE);
	} else if (!strcmp(commands[1], "-license")) {
		char *license_file = "/etc/klearnel/LICENSE";
		if (access(license_file, F_OK) == 0){
			char command[256] = "";
			snprintf(command, 256, "cat %s | more", license_file);
			system(command);
		} else {
			printf("The license file doesn't seem to be installed.\"n"
				"You can find it at https://github.com/Klearnel-Devs/klearnel/blob/master/LICENSE\n");
		}
	} else if (!strcmp(commands[1], "-stop")) {
		stop_action();

	} else if (!strcmp(commands[1], "-flush")) {
		if (_kill_mutex() < 0) {
			printf("Unable to flush Klearnel mutex\n");
		} else {
			printf("Klearnel mutex is flushed\n"
				"You can restart it normally\n");
		}
	} else if (!strcmp(commands[1], "-help")) {
		printf("\n\e[4mKlearnel commands:\e[24m\n\n");
		printf(" \e[1m-add-to-qr <path-to-file>\e[0m:\n\t Add a new file to the quarantine\n\n");
		printf(" \e[1m-rm-from-qr <filename>\e[0m:\n\t Remove the specified file from the quarantine"
			"\n\t \e[1m\e[31mWARNING\e[0m: this command will definitively remove the file!\n\n");
		printf(" \e[1m-rm-all-from-qr\e[0m:\n\t Remove all files from the quarantine"
			"\n\t \e[1m\e[31mWARNING\e[0m: this command will definitively remove the files!\n\n");
		printf(" \e[1m-get-qr-list\e[0m:\n\t Display the file currently stored in quarantine\n");
		printf(" \e[1m-get-qr-info <filename>\e[0m:\n\t NOT YET IMPLEMENTED\n\n");
		printf(" \e[1m-restore-from-qr <filename>\e[0m:\n\t Restore the specified file to its original location\n\n");
		printf(" \e[1m-restore-all-from-qr\e[0m:\n\t Restore all files to their original locations\n\n");
		printf(" \e[1m-add-to-scan <file/folder path>\e[0m:\n\t Add the specified file or folder to the scanner watch list"
			"\n\t \e[1m\e[33mNOTE\e[0m: this command will prompt you for each action to apply to the new item. It can take a few minutes to complete.\n\n");
		printf(" \e[1m-rm-from-scan <file/folder path>\e[0m: \n\t Remove the specified file/folder from the scanner watch list\n\n");	
		printf(" \e[1m-get-scan-list\e[0m:\n\t Display the elements in the scanner watch list\n\n");
		printf(" \e[1m-force-scan\e[0m:\n\t Force Scanner to execute scan tasks\n\n");
		printf(" \e[1m-view-rt-log\e[0m:\n\t Display the current klearnel's log in real time\n\n");
		printf(" \e[1m-license\e[0m:\n\t Display the klearnel license terms\n\n");
		printf(" \e[1m-flush\e[0m:\n\t Flush IPC's created by Klearnel services"
			"\n\t To use only when services crashed\n\n");
		printf(" \e[1m-start\e[0m:\n\t Start Klearnel services"
			"\n\t \e[1m\e[33mNOTE\e[0m: you need to be root to start the services\n\n");
		printf(" \e[1m-restart\e[0m:\n\t Restart the Klearnel services"
			"\n\t \e[1m\e[33mNOTE\e[0m: you need to be root to restart the services\n\n");
		printf(" \e[1m-stop\e[0m:\n\t Stop Klearnel service\n\n");
		printf(" \e[1m-help\e[0m:\n\t Display this help message\n\n");
		printf("\n Copyright (C) 2014, 2015 Klearnel-Devs\n\n\e[0m");
	} else {
		fprintf(stderr, "Unknown command\n"
			"Try \"klearnel -help\" to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
