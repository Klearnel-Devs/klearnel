/*
 * Contains all routines to execute actions linked to Quarantine
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
int qr_query(int nb, char **commands, int action) 
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
			QrList *qr_list = calloc(1, sizeof(QrList));
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
			print_qr(&qr_list);
			clear_qr_list(&qr_list);
			free(list_path);
			break;
		case QR_RM_ALL:
		case QR_REST_ALL:
		case KL_EXIT:
			snprintf(query, len, "%d:0", action);
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
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