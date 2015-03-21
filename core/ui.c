/*
 * Contains all routines to execute actions linked to args in cli
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <core/ui.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>

/*
 * Allow to send a query to the quarantine and execute the related action
 * Return 0 on success and -1 on error
 */
int _qr_query(int nb, char **commands, int action) 
{
	int len, s_cl, i;
	char *query, *res;;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("[UI] Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, QR_SOCK, strlen(QR_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
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
	if ((query == NULL) || (res == NULL)) {
		perror("[UI] Unable to allocate memory");
		goto error;
	}
	i = 2;
	switch (action) {
		case QR_ADD:
		case QR_RM:
		case QR_REST:
			
			do {
				int c_len = strlen(commands[i]) + 1;
				snprintf(query, len, "%d:%d", action, c_len);
				if (write(s_cl, query, len) < 0) {
					perror("[UI] Unable to send query");
					goto error;
				}
				if (read(s_cl, res, 2) < 0) {
					perror("[UI] Unable to get query result");
					goto error;
				}
				
				if (write(s_cl, commands[i], c_len) < 0) {
					perror("[UI] Unable to send args of the query");
					goto error;
				}
				if (read(s_cl, res, 2) < 0) {
					perror("[UI] Unable to get query result");
					goto error;					
				}
				if (read(s_cl, res, 2) < 0) {
					perror("[UI] Unable to get query result");
					goto error;
				}
				if (!strcmp(res, SOCK_ACK))
					i++;
				if (!strcmp(res, SOCK_UNK)) 
					fprintf(stderr, "File not found in quarantine");
			} while (commands[i]);
			break;
		case QR_INFO: ;
			int c_len = strlen(commands[2]);
			snprintf(query, len, "%d:%d", action, c_len);
			NOT_YET_IMP;
			break;
		case QR_LIST:
			snprintf(query, len, "%d:0", action);
			char *list_path = malloc(PATH_MAX);
			QrSearchTree qr_list = NULL;
			int fd;
			if (!list_path) {
				perror("[UI] Unable to allocate memory");
				goto error;
			}
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				free(list_path);
				goto error;
			} 
			if (read(s_cl, res, 2) < 0) {
				perror("[UI] Unable to get query result");
				goto error;
			}
			if (read(s_cl, list_path, PATH_MAX) < 0) {
				perror("[UI] Unable to get query result");
				free(list_path);
				goto error;	
			} 
			if (strcmp(list_path, SOCK_ABORTED) == 0) {
				perror("[UI] Action get-qr-list couldn't be executed");
				free(list_path);
				goto error;
			} 

			fd = open(list_path, O_RDONLY, S_IRUSR);
			if (fd < 0) {
				perror("[UI] Unable to open qr list file");
				free(list_path);
				goto error;
			}
			
			load_tmp_qr(&qr_list, fd);
			close(fd);

			print_qr(qr_list);
			clear_qr_list(qr_list);
			
			free(list_path);
			break;
		default:
			fprintf(stderr, "[UI] Unknown action");
	}
	free(query);
	free(res);
	close(s_cl);
	return 0;
error:
	free(query);
	free(res);
	close(s_cl);
	return -1;
	
}

/* Main function that calls the right routine 
 * acordingly to the passed arguments
 */ 
void execute_commands(int nb, char **commands)
{
	if (!strcmp(commands[1], "add-to-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel add-to-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		_qr_query(nb, commands, QR_ADD);
	} else if (!strcmp(commands[1], "rm-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				 "Correct syntax is: klearnel rm-fom-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		_qr_query(nb, commands, QR_RM);
	} else if (!strcmp(commands[1], "get-qr-list")) {
		_qr_query(nb, commands, QR_LIST);
	} else if (!strcmp(commands[1], "get-qr-info")) {
		if (nb < 3) {
			fprintf(stderr, "Element to detail missing\n"
				 "Correct syntax is: klearnel get-qr-info <filename>\n");
			exit(EXIT_FAILURE);
		}
		_qr_query(nb, commands, QR_INFO);
	} else if (!strcmp(commands[1], "restore-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to restore missing\n"
				 "Correct syntax is: klearnel restore-from-qr <filename>\n");
			exit(EXIT_FAILURE);
		}
		_qr_query(nb, commands, QR_REST);
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
	} else if (!strcmp(commands[1], "license")) {
		NOT_YET_IMP;
		printf("See the LICENSE file located in /etc/klearnel\n");
	} else if (!strcmp(commands[1], "help")) {
		printf("\n\e[4mKlearnel commands:\e[24m\n\n");
		printf(" - \e[1madd-to-qr <path-to-file>\e[21m:\n\t Add a new file to the quarantine\n");
		printf(" - \e[1mrm-from-qr <filename>\e[21m:\n\t Remove the specified file from the quarantine"
			"\n\tWARNING: this command will definitively remove the file!\n");
		printf(" - \e[1mget-qr-list\e[21m:\n\t Display the file currently stored in quarantine\n");
		printf(" - \e[1mget-qr-info <filename>\e[21m:\n\t NOT YET IMPLEMENTED\n");
		printf(" - \e[1mrestore-from-qr <filename>\e[21m:\n\t Restore the specified file to its original location\n");
		printf(" - \e[1mview-rt-log\e[21m:\n\t Display the current klearnel's log in real time\n");
		printf(" - \e[1mlicense\e[21m:\n\t Display the klearnel license terms\n");
		printf(" - \e[1mhelp\e[21m:\n\t Display this help message\n");
		printf("\nCopyright (C) 2014, 2015 Klearnel-Devs\n\n");
	} else {
		fprintf(stderr, "Unknown command\n"
			"Try \"klearnel help\" to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
