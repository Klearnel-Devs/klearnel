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
void _search_expired(QrList **list, QrListNode *listNode, time_t now)
{
	if (listNode == NULL) {
		return;
	}
  	if (listNode->data.d_expire < now) {
		rm_file_from_qr(list, listNode->data.f_name);
	}
	_search_expired(list, listNode->next, now);
	
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
	time_t now = time(NULL);
	_search_expired(list, (*list)->first, now);
	write_to_log(DEBUG, "%s successfully completed", __func__);
	clear_qr_list(list);
	return;
}

/* Call the action related to the "action" arg 
 * If action has been executed correctly return the new qr_list, 
 * if not return the unchanged list or NULL
 */
int _call_related_action(QrList **list, const int action, char *buf, const int s_cl) 
{
	switch (action) {
		case QR_ADD: 
			if (add_file_to_qr(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list(list);
				*list = calloc(1, sizeof(QrList));
				load_qr(list);
				return 0;
			} else {
				SOCK_ANS(s_cl, SOCK_ACK);
			}
			break;
		case QR_RM:
		case QR_RM_ALL:
			if (rm_file_from_qr(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list(list);
				*list = calloc(1, sizeof(QrList));
				load_qr(list);
				return 0;
			} else {
				SOCK_ANS(s_cl, SOCK_ACK);
			}
			if (action == QR_RM_ALL) {
				return 1;
			}
			break;
		case QR_REST:
		case QR_REST_ALL:
			if (restore_file(list, buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list(list);
				*list = calloc(1, sizeof(QrList));
				load_qr(list);
				return 0;
			} else {
				SOCK_ANS(s_cl, SOCK_ACK);
			}
			if (action == QR_REST_ALL) {
				return 1;
			}
			break;
		case QR_LIST:
		case QR_LIST_RECALL: ;
			time_t timestamp = time(NULL);
			int tmp_stock;
			char *path_to_list = malloc(sizeof(char)*(sizeof(timestamp)+20));
			char *file = malloc(sizeof(char)*(sizeof(timestamp)+30));
			if (!path_to_list || !file) {
				write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				return 0;
			}
			sprintf(path_to_list, "%s", QR_TMP);
			if (access(path_to_list, F_OK) == -1) {
				if (mkdir(path_to_list, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
					write_to_log(WARNING, "%s - %d - %s:%s", __func__, __LINE__, "Unable to create the tmp_stock folder", path_to_list);
					if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
						write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
					}
					free(path_to_list);
					return 0;
				}
			}
			sprintf(file, "%s/%d", path_to_list, (int)timestamp);
			tmp_stock = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH);
			if (tmp_stock < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open tmp_stock");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				free(path_to_list);
				free(file);
				return 0;
			}
			save_qr_list(list, tmp_stock);
			close(tmp_stock);
			write(s_cl, file, PATH_MAX);
			free(path_to_list);
			free(file);
			if (action == QR_LIST_RECALL) {
				return 1;
			}
			break;
		case QR_INFO:
			NOT_YET_IMP;
			break;
		case KL_EXIT:
			LOG(INFO, "Received KL_EXIT command");			
			clear_qr_list(list);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		default: write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "DEFAULT ACTION");
	}
	return 0;
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
	int c_len = 20;
	int action = 0;
	struct sockaddr_un server;
	QrList *list = calloc(1, sizeof(QrList));
	load_qr(&list);

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
		int res, recall = 0;

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
				do {
					if (get_data(s_cl, &action, &buf, c_len) < 0) {
						free(buf);
						close(s_cl);
						write_to_log(NOTIFY, "%s - %d - %s", __func__, __LINE__, "_get_data FAILED");
						continue;
					}
					recall = _call_related_action(&list, action, buf, s_cl);
				} while (recall != 0);
				free(buf);
				close(s_cl);
			}
		} else {
			_expired_files(&list);
		}
	} while (action != KL_EXIT);
	close(s_srv);
	unlink(server.sun_path);
}

/* Main function of qr-worker process */
void qr_worker()
{
	_get_instructions();
	exit(EXIT_SUCCESS);
}