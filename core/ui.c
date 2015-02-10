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
	timeout.sec 	= SOCK_TO;
	timeout.usec 	= 0;

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
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, 
		sizeof(timeout)) < 0)
		write_to_log(WARNING, "[UI] Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
		sizeof(timeout)) < 0)
		write_to_log(WARNING, "[UI] Unable to set timeout for sending operations");
	
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
			NOT_YET_IMP;
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
				 "Correct syntax is: klearnel add-to-qr <filename or dirname>\n");
		}
		_qr_query(nb, commands, QR_ADD);
	} else if (!strcmp(commands[1], "rm-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to remove missing\n"
				 "Correct syntax is: klearnel rm-fom-qr <filename or dirname>\n");
		}
		_qr_query(nb, commands, QR_RM);
	} else if (!strcmp(commands[1], "get-qr-list")) {
		_qr_query(nb, commands, QR_LIST);
	} else if (!strcmp(commands[1], "get-info")) {
		if (nb < 3) {
			fprintf(stderr, "Element to detail missing\n"
				 "Correct syntax is: klearnel get-info <filename or dirname>\n");
		}
		_qr_query(nb, commands, QR_INFO);
	} else if (!strcmp(commands[1], "restore-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to restore missing\n"
				 "Correct syntax is: klearnel restore-qr <filename or dirname>\n");
		}
		_qr_query(nb, commands, QR_REST);
	} else if (!strcmp(commands[1], "-h")) {
		NOT_YET_IMP; /* IMPORTANT: TO DO NEARLY */
	} else {
		fprintf(stderr, "Unknow command\n"
			"Try klearnel -h to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
