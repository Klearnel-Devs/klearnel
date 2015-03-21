/*
 * Process that associates tasks with required quarantine actions
 * Forked from Klearnel, forks to accomplish certain tasks
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>

/*
 * Searches QR list and deletes a file who's date is older than todays date time
 * Calls to rm_file_from_qr to delete file physically, logically
 */
void _search_expired(QrList **list, QrListNode *listNode, int *removed, time_t now)
{
	if (listNode == NULL)
      		return;
      	if (listNode->data.d_expire < now) {
		rm_file_from_qr(list, listNode->data.f_name);
		*removed += 1;
	} else {
		_search_expired(list, listNode->next, removed, now);
	}
	
	return;
}

/*
 * Function of process who is tasked with deleting files
 * earmarked by a deletion date older than todays date time
 * Loops until no more expired files are detected
 */
void _expired_files(QrList **list)
{
	(*list) = calloc(1, sizeof(QrList));
	load_qr(list);
	if(list == NULL) {
		write_to_log(NOTIFY, "%s - Quarantine List is empty", __func__);
		return;
	}
	int removed;
	time_t now;
	do {
		removed = 0;
		now = time(NULL);
		_search_expired(list, (*list)->first, &removed, now);
	} while ( removed != 0 );
	write_to_log(DEBUG, "%s successfully completed", __func__);
	clear_qr_list(list);
	return;
}

/* Get data from socket "sock" and put it in buffer "buf"
 * Return number of char read if >= 0, else -1
 */
static int _get_data(const int sock, int *action, char **buf)
{
	int c_len = 20;
	char *a_type = malloc(c_len);
	int len;
	if (a_type == NULL) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}

	if (read(sock, a_type, c_len) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Error while receiving data through socket");
		return -1;
	}

	if (SOCK_ANS(sock, SOCK_ACK) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send ack in socket");
		free(a_type);
		return -1;
	}

	*action = atoi(strtok(a_type, ":"));
	len = atoi(strtok(NULL, ":"));
	
	if (len > 0) {
		*buf = malloc(sizeof(char)*len);
		if (*buf == NULL) {
			if (SOCK_ANS(sock, SOCK_RETRY) < 0)
				write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send retry");
			write_to_log(FATAL,"%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
			free(a_type);
			return -1;
		}

		if (read(sock, *buf, len) < 0) {
			if (SOCK_ANS(sock, SOCK_NACK) < 0)
				write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send nack");
			write_to_log(FATAL,"%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
			free(a_type);
			return -1;			
		}

		if(SOCK_ANS(sock, SOCK_ACK) < 0) {
			write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send ack");
			free(a_type);
			return -1;
		}
	}
	write_to_log(DEBUG, "%s successfully completed", __func__);
	free(a_type);
	return len;
}

/* Call the action related to the "action" arg 
 * If action has been executed correctly return the new qr_list, 
 * if not return the unchanged list or NULL
 */
void _call_related_action(QrList **list, const int action, char *buf, const int s_cl) 
{
	(*list) = calloc(1, sizeof(QrList));
	load_qr(list);
	QrList *save = *list;
	switch (action) {
		case QR_ADD: 
			if (add_file_to_qr(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				*list = save;
				return;
			} else {
				clear_qr_list(list);
			}
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_RM:			
			if (rm_file_from_qr(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				*list = save;
				return;
			} else {
				clear_qr_list(list);
			}
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_REST:
			if (restore_file(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				*list = save;
				return;
			} else {
				clear_qr_list(list);
			}
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_LIST: ;
			time_t timestamp = time(NULL);
			int tmp_stock;
			char *path_to_list = malloc(sizeof(char)*(sizeof(timestamp)+20));
			char *file = malloc(sizeof(char)*(sizeof(timestamp)+30));
			if (!path_to_list || !file) {
				write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				return;
			}
			sprintf(path_to_list, "/tmp/.klearnel/%d", (int)timestamp);
			if (access(path_to_list, F_OK) == -1) {
				if (mkdir(path_to_list, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to create the tmp_stock folder");
					if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
						write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
					}
					free(path_to_list);
					return;
				}
			}
			sprintf(file, "%s/qr_stock", path_to_list);
			tmp_stock = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH);
			if (tmp_stock < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open tmp_stock");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				free(path_to_list);
				free(file);
				return;
			}
			save_qr_list(list, tmp_stock);
			close(tmp_stock);
			write(s_cl, file, PATH_MAX);
			free(path_to_list);
			free(file);
			break;
		case QR_INFO:
			NOT_YET_IMP;
			break;
		case KL_EXIT:
			if (list != NULL) {
				clear_qr_list(list);
			}
			SOCK_ANS(s_cl, SOCK_ACK);			
			break;
		default: write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "DEFAULT ACTION");;
	}
	write_to_log(DEBUG, "%s successfully completed", __func__);
}

/* Get instructions via Unix domain sockets
 * and execute actions accordingly
 * 
 * For client side, follow these steps:
 * - Connect to the socket
 * - Send the action to execute (see quarantine.h)
 * with the length of second arg (format is action_num:length_sec_arg)
 * - Wait for SOCK_ARG
 * - Send the second arg (if applicable)
 * - Wait for action result
 */
void _get_instructions()
{
	int len, s_srv, s_cl;
	int action = 0;
	struct sockaddr_un server;
	QrList *list = NULL;

	if ((s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open the socket");
		return;
	}
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, QR_SOCK, strlen(QR_SOCK) + 1);
	unlink(server.sun_path);
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if(bind(s_srv, (struct sockaddr *)&server, len) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to bind the socket");
		return;
	}
	listen(s_srv, 10);

	do {
		struct timeval to_socket;
		to_socket.tv_sec 	= SOCK_TO;
		to_socket.tv_usec = 0;
		struct timeval to_select;
		to_select.tv_sec 	= SEL_TO;
		to_select.tv_usec = 0;
		fd_set fds;
		int res;

		FD_ZERO (&fds);
		FD_SET (s_srv, &fds);

		struct sockaddr_un remote;
		char *buf = NULL;

		res = select (s_srv + 1, &fds, NULL, NULL, &to_select);
		if (res > 0) {
			if (FD_ISSET(s_srv, &fds)) {
				len = sizeof(remote);
				if ((s_cl = accept(s_srv, (struct sockaddr *)&remote, (socklen_t *)&len)) == -1) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to accept the connection");
					continue;
				}
				if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to set timeout for reception operations");

				if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to set timeout for sending operations");		

				if (_get_data(s_cl, &action, &buf) < 0) {
					free(buf);
					close(s_cl);
					write_to_log(NOTIFY, "%s - %d - %s", __func__, __LINE__, "_get_data FAILED");
					continue;
				}
				_call_related_action(&list, action, buf, s_cl);
				free(buf);
				close(s_cl);
			}
		} else {
			_expired_files(&list);
		}
	} while (action != KL_EXIT);
	close(s_srv);
	unlink(server.sun_path);
	write_to_log(DEBUG, "%s successfully completed", __func__);
}

/* Main function of qr-worker process */
void qr_worker()
{
	_get_instructions();
}