/*
 * Contains all routines to execute actions linked to args in cli
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <core/ui.h>
#include <quarantine/quarantine.h>
#include <core/scanner.h>
#include <logging/logging.h>


/* Main function that calls the right routine 
 * acordingly to the passed arguments
 */ 
void execute_commands(int nb, char **commands)
{
	if (!strcmp(commands[1], "add-to-qr")) {
		int i;
		struct stat new_s;
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel add-to-qr <filename>\n");
			exit(EXIT_FAILURE);
		} else {
			for (i = 2; i < nb; i++) {
				if (stat(commands[i], &new_s) < 0) {
					printf("File %s could not be found\n", commands[i]);
					return;
				}
			}
		}
		qr_query(nb, commands, QR_ADD);
	} else if (!strcmp(commands[1], "rm-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				 "Correct syntax is: klearnel rm-fom-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(nb, commands, QR_RM);
	} else if (!strcmp(commands[1], "rm-all-from-qr")) {
		qr_query(nb, commands, QR_RM_ALL);
	} else if (!strcmp(commands[1], "get-qr-list")) {
		qr_query(nb, commands, QR_LIST);
	} else if (!strcmp(commands[1], "get-qr-info")) {
		if (nb < 3) {
			fprintf(stderr, "Element to detail missing\n"
				 "Correct syntax is: klearnel get-qr-info <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(nb, commands, QR_INFO);
	} else if (!strcmp(commands[1], "restore-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to restore missing\n"
				 "Correct syntax is: klearnel restore-from-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		qr_query(nb, commands, QR_REST);
	} else if (!strcmp(commands[1], "restore-all-from-qr")) {
		qr_query(nb, commands, QR_REST_ALL);
	} else if (!strcmp(commands[1], "view-rt-log")) {
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
	} else if (!strcmp(commands[1], "add-to-scan")) {
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel add-to-scan <file/folder path>\n");
			exit(EXIT_FAILURE);
		}
		scan_query(nb, commands, SCAN_ADD);
	} else if (!strcmp(commands[1], "rm-from-scan")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				"Correct syntax is: klearnel rm-from-scan <file/folder path to remove>\n");
			exit(EXIT_FAILURE);
		}
		scan_query(nb, commands, SCAN_RM);
	} else if (!strcmp(commands[1], "get-scan-list")) {
		scan_query(nb, commands, SCAN_LIST);
	} else if (!strcmp(commands[1], "license")) {
		NOT_YET_IMP;
		printf("See the LICENSE file located in /etc/klearnel\n");
	} else if (!strcmp(commands[1], "stop")) {
		printf("Stopping Klearnel services\n\n");
		if (qr_query(nb, commands, KL_EXIT) != 0) {
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
	} else if (!strcmp(commands[1], "help")) {
		printf("\n\e[4mKlearnel commands:\e[24m\n\n");
		printf(" - \e[1madd-to-qr <path-to-file>\e[21m:\n\t Add a new file to the quarantine\n");
		printf(" - \e[1mrm-from-qr <filename>\e[21m:\n\t Remove the specified file from the quarantine"
			"\n\tWARNING: this command will definitively remove the file!\n");
		printf(" - \e[1mrm-all-from-qr\e[21m:\n\t Remove all files from the quarantine"
			"\n\tWARNING: this command will definitively remove the files!\n");
		printf(" - \e[1mget-qr-list\e[21m:\n\t Display the file currently stored in quarantine\n");
		printf(" - \e[1mget-qr-info <filename>\e[21m:\n\t NOT YET IMPLEMENTED\n");
		printf(" - \e[1mrestore-from-qr <filename>\e[21m:\n\t Restore the specified file to its original location\n");
		printf(" - \e[1mrestore-all-from-qr\e[21m:\n\t Restore all files to their original locations\n");
		printf(" - \e[1madd-to-scan <file/folder path>\e[21m:\n\t Add the specified file or folder to the scanner watch list"
			"\n\tNOTE: this command will prompt you for each action to apply to the new item. It can take a few minutes to complete.\n");
		printf(" - \e[1mrm-from-scan <file/folder path>\e[21m: \n\t Remove the specified file/folder from the scanner watch list\n");	
		printf(" - \e[1mview-rt-log\e[21m:\n\t Display the current klearnel's log in real time\n");
		printf(" - \e[1mlicense\e[21m:\n\t Display the klearnel license terms\n");
		printf(" - \e[1mstart\e[21m:\n\t Start Klearnel service\n");
		printf(" - \e[1mstop\e[21m:\n\t Stop Klearnel service\n");
		printf(" - \e[1mhelp\e[21m:\n\t Display this help message\n");
		printf("\nCopyright (C) 2014, 2015 Klearnel-Devs\n\n");
	} else {
		fprintf(stderr, "Unknown command\n"
			"Try \"klearnel help\" to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
