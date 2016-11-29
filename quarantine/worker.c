/*-------------------------------------------------------------------------*/
/**
   \file	worker.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Quarantine file

   Process that associates tasks with required quarantine actions
   Forked from Klearnel, forks to accomplish certain tasks
*/
/*--------------------------------------------------------------------------*/
#include <global.h>
#include <config/config.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>

/*------------------------------------------------------------------------*/
/**
  \brief        Call the action related to the "action" arg 
  \param 	action  The action to take
  \param        buf 	Command received by socket
  \param 	s_cl 	The socket connection
  \return       0 on success, -1 on error

 */
/*--------------------------------------------------------------------------*/
int _call_related_action(const int action, char *buf, const int s_cl) 
{
	switch (action) {
		case QR_ADD: 
			if (add_file_to_qr(buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list();
				load_qr();
				return 0;
			} else {
				if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
					LOG(URGENT, "Unable to send ACK");
					return -1;
				}
			}
			break;
		case QR_RM:
		case QR_RM_ALL:
			if (rm_file_from_qr(buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list();
				load_qr();
				return 0;
			} else {
				if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
					LOG(URGENT, "Unable to send ACK");
					return -1;
				}
			}
			if (action == QR_RM_ALL) {
				return 1;
			}
			break;
		case QR_REST:
		case QR_REST_ALL:
			if (restore_file(buf) < 0) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				clear_qr_list();
				load_qr();
				return 0;
			} else {
				if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
					LOG(URGENT, "Unable to send ACK");
					return -1;
				}
			}
			if (action == QR_REST_ALL) {
				return 1;
			}
			break;
		case QR_LIST:
		case QR_LIST_RECALL: ;
			if (is_empty() < 1) {
				if (write(s_cl, VOID_LIST, strlen(VOID_LIST)) < 0) {
					LOG(WARNING, "Unable to send VOID_LIST");
					return -1;
				}
				return 0;
			}
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
			int oldmask = umask(0);
			if (access(path_to_list, F_OK) == -1) {
				if (mkdir(path_to_list, ALL_RWX)) {
					write_to_log(WARNING, "%s - %d - %s:%s", __func__, __LINE__, "Unable to create the tmp_stock folder", path_to_list);
					if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
						write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
					}
					free(path_to_list);
					return 0;
				}
			}
			sprintf(file, "%s/%d", path_to_list, (int)timestamp);
			tmp_stock = open(file, O_WRONLY | O_TRUNC | O_CREAT, ALL_RWX);
			if (tmp_stock < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open tmp_stock");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				free(path_to_list);
				free(file);
				return 0;
			}
			save_qr_list(tmp_stock);
			close(tmp_stock);
			umask(oldmask);
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
			write_to_log(INFO, "Quarantine received stop command");		
			exit_quarantine();
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		case RELOAD_CONF:
			reload_config();
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		default: write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "DEFAULT ACTION");
	}
	return 0;
}
 
/*-------------------------------------------------------------------------*/
/**
  \brief        Get instructions via Unix domain sockets
  \return       void

  Get instructions via Unix domain sockets and execute actions accordingly
  For client side, follow these steps:
  - Connect to the socket
  - Send the action to execute (see quarantine.h)
  with the length of second arg (format is action_num:length_sec_arg)
  - Wait for SOCK_ARG
  - Send the second arg (if applicable)
  - Wait for action result
 */
/*--------------------------------------------------------------------------*/
void _get_instructions()
{
	int len, s_srv, s_cl;
	int c_len = 20;
	int action = 0;
	struct sockaddr_un server;
	load_qr();
	int oldmask = umask(0);

	if ((s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open the socket");
		return;
	}

	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, QR_SOCK, sizeof(server.sun_path));
	if (unlink(server.sun_path) != 0) {
		if(errno != ENOENT) {
			write_to_log(URGENT, "%s:%d: Unable to remove the old quarantine socket, Error: %d", 
				__func__, __LINE__, errno);
		}
	}

	len = sizeof(server.sun_path) + sizeof(server.sun_family);
	if(bind(s_srv, (struct sockaddr *)&server, len) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to bind the socket");
		return;
	}

	umask(oldmask);
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
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, "Unable to accept the connection");
					continue;
				}
				if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, "Unable to set timeout for reception operations");

				if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, "Unable to set timeout for sending operations");		
				do {
					if (get_data(s_cl, &action, &buf, c_len) < 0) {
						free(buf);
						close(s_cl);
						write_to_log(NOTIFY, "%s - %d - %s", __func__, __LINE__, "_get_data FAILED");
						continue;
					}
					recall = _call_related_action(action, buf, s_cl);
				} while (recall != 0);
				free(buf);
				close(s_cl);
			}
		} else {
			expired_files();
		}
	} while (action != KL_EXIT);
	shutdown(s_srv, SHUT_RDWR);
	close(s_srv);
}

void qr_worker()
{
	_get_instructions();
	exit(EXIT_SUCCESS);
}