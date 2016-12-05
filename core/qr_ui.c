/*-------------------------------------------------------------------------*/
/**
   \file	qr_ui.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Quarantine UI file

   Contains all routines to execute actions linked to Quarantine
*/
/*--------------------------------------------------------------------------*/
#include <global.h>
#include <core/ui.h>
#include <quarantine/quarantine.h>

int qr_query(char **commands, int action) 
{
	int len, s_cl, i;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("[UI] Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, QR_SOCK, sizeof(remote.sun_path));
	len = sizeof(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		if (action == KL_EXIT) {
			printf("Quarantine service is not running\n");
			return -2;
		}
		perror("[UI] Unable to connect the qr_sock");
		return -1;
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
			do {
				if (access(commands[i], F_OK) == -1) {
					printf("%s: Unable to find %s.\n"
						"Please check if the path is correct.\n", __func__, commands[i]);
					sprintf(query, "%s", VOID_LIST);
					if (write(s_cl, query, strlen(query)) < 0) {
						perror("QR-UI: Unable to send file location state");
						goto error;
					}
					if (read(s_cl, res, 1) < 0) {
						perror("QR-UI: Unable to get location result");
					}
					goto error;
				}
				i++;
			} while (commands[i]);
			i = 2;
		case QR_RM:
		case QR_REST:
			do {
				int c_len = strlen(commands[i]) + 1;
				snprintf(query, len, "%d:%d", action, c_len);
				if (write(s_cl, query, len) < 0) {
					perror("[UI] Unable to send query");
					goto error;
				}
				if (read(s_cl, res, 1) < 0) {
					perror("[UI] Unable to get query result");
					goto error;
				}
				
				if (write(s_cl, commands[i], c_len) < 0) {
					perror("[UI] Unable to send args of the query");
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
				if (!strcmp(res, SOCK_ACK)) {
					switch(action) {
						case QR_ADD:  printf("File %s successfully added to QR\n", commands[i]); break;
						case QR_RM:   printf("File %s successfully deleted from QR\n", commands[i]); break;
						case QR_REST: printf("File %s successfully restored\n", commands[i]); break;
					}
					i++;
				} else if (!strcmp(res, SOCK_ABORTED)) {
					switch(action) {
						case QR_ADD:  printf("File %s could not be added to QR\n", commands[i]); break;
						case QR_RM:   printf("File %s could not be deleted from QR\n", commands[i]); break;
						case QR_REST: printf("File %s could not be restored\n", commands[i]); break;
					}
					i++;
				} else if (!strcmp(res, SOCK_UNK)) {
					fprintf(stderr, "File not found in quarantine\n");
					i++;
				}
			} while (commands[i]);
			break;
		case QR_INFO: ;
			int c_len = strlen(commands[2]);
			snprintf(query, len, "%d:%d", action, c_len);
			NOT_YET_IMP;
			break;
		case QR_RM_ALL:
		case QR_REST_ALL:
		case QR_LIST:
			if (action != QR_LIST) {
				snprintf(query, len, "%d:0", QR_LIST_RECALL);
			} else {
				snprintf(query, len, "%d:0", action);
			}
			clock_t begin;
			begin = clock();

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
			if (read(s_cl, res, 1) < 0) {
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
				printf("The Quarantine doesn't contain any file\n");
				free(list_path);
				goto error;
			}
			load_tmp_qr(&qr_list, fd);
			close(fd);
			if (unlink(list_path))
				printf("Unable to remove temporary quarantine file: %s", list_path);
			if (action == QR_LIST) {
				clock_t end;
				double spent;
				print_qr(&qr_list);
				end = clock();
				spent = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("\nQuery executed in: %.2lf seconds\n", spent);
				goto out;
			} else {
				TMP_LIST_FOREACH(&qr_list, first, next, cur) {
					char *next_query = malloc(len);
					if (next_query == NULL) {
						perror("[UI] Unable to allocate memory");
						goto error;
					}
					int c_len = strlen(cur->data.f_name) + 1;
					if ( cur->next == NULL ) {
						if ( action == QR_RM_ALL ) {
							snprintf(next_query, len, "%d:%d", QR_RM, c_len);
						} else {
							snprintf(next_query, len, "%d:%d", QR_REST, c_len);
						}
					} else {
						snprintf(next_query, len, "%d:%d", action, c_len);
					}
					if (write(s_cl, next_query, len) < 0) {
						perror("[UI] Unable to send query");
						goto error;
					}
					if (read(s_cl, res, 1) < 0) {
						perror("[UI] Unable to get query result");
						goto error;
					}
					if (write(s_cl, cur->data.f_name, c_len) < 0) {
						perror("[UI] Unable to send args of the query");
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
					if (!strcmp(res, SOCK_ACK)) {
						switch (action) {
							case QR_RM_ALL: printf("File %s successfully deleted from QR\n", cur->data.f_name); break;
							case QR_REST_ALL: printf("File %s successfully restored\n", cur->data.f_name); break;
						}
					} else if (!strcmp(res, SOCK_ABORTED)) {
						switch (action) {
							case QR_RM_ALL: printf("File %s could not be deleted from QR\n", cur->data.f_name); break;
							case QR_REST_ALL: printf("File %s could not be restored\n", cur->data.f_name); break;
						}
					} else if (!strcmp(res, SOCK_UNK)) {
						fprintf(stderr, "File not found in quarantine\n");
					}
					free(next_query);
				}
			}
out:
			clear_tmp_qr_list(&qr_list);
			free(list_path);
			free(qr_list);
			break;
		case KL_EXIT:
			snprintf(query, len, "%d:0", action);
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
			break;
		default:
			fprintf(stderr, "[UI] Unknown action");
	}
	free(query);
	free(res);
	shutdown(s_cl, SHUT_RDWR);
	close(s_cl);
	return 0;
error:
	free(query);
	free(res);
	shutdown(s_cl, SHUT_RDWR);
	close(s_cl);
	return -1;
	
}