 /**
 * \file networker.c
 * \brief Networker File
 * \author Copyright (C) 2014, 2015 Klearnel-Devs 
 * 
 * This file contains all functions that are needed
 * to communicate with klearnel-manager
 * through the network socket
 *
 */
#include <global.h>
#include <logging/logging.h>
#include <quarantine/quarantine.h>
#include <core/scanner.h>
#include <net/network.h>




int _execute_qr_action(const char *buf, const int c_len, const int action, const int net_sock) 
{
	int len, s_cl;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		LOG(FATAL, "Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, QR_SOCK, strlen(QR_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		LOG(FATAL, "Unable to connect the qr_sock");
		goto error;
	}
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if ((query == NULL) || (res == NULL)) {
		LOG(FATAL, "Unable to allocate memory");
		goto error;
	}
	switch (action) {
		case QR_ADD:
			if (access(buf, F_OK) == -1) {
				write_to_log(WARNING, "%s: Unable to find %s.\n"
					"Please check if the path is correct.\n", __func__, buf);
				goto error;
			}
		case QR_RM:
		case QR_REST: ;
			snprintf(query, len, "%d:%d", action, c_len);
			if (write(s_cl, query, len) < 0) {
				LOG(URGENT, "Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;
			}

			if (write(s_cl, buf, c_len) < 0) {
				LOG(URGENT, "Unable to send args of the query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;
			}
			if (!strcmp(res, SOCK_ACK)) {
				switch(action) {
					case QR_ADD:  write_to_log(INFO, "File %s successfully added to QR\n", buf); break;
					case QR_RM:   write_to_log(INFO, "File %s successfully deleted from QR\n", buf); break;
					case QR_REST: write_to_log(INFO, "File %s successfully restored\n", buf); break;
				}
			} else if (!strcmp(res, SOCK_ABORTED)) {
				switch(action) {
					case QR_ADD:  write_to_log(INFO, "File %s could not be added to QR\n", buf); break;
					case QR_RM:   write_to_log(INFO, "File %s could not be deleted from QR\n", buf); break;
					case QR_REST: write_to_log(INFO, "File %s could not be restored\n", buf); break;
				}
			} else if (!strcmp(res, SOCK_UNK)) {
				LOG(WARNING, "File not found in quarantine");
			}		
			break;
		case QR_RM_ALL:
		case QR_REST_ALL:
		case QR_LIST:
			if (action != QR_LIST) {
				snprintf(query, len, "%d:0", QR_LIST_RECALL);
			} else {
				snprintf(query, len, "%d:0", action);
			}
			
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
				perror("[UI] Unable to open qr list file");
				free(list_path);
				goto error;
			}
			load_tmp_qr(&qr_list, fd);
			close(fd);
			if (unlink(list_path))
				printf("Unable to remove temporary quarantine file: %s", list_path);
			if (action == QR_LIST) {
				LIST_FOREACH(&qr_list, first, next, cur) {
					char size[20];
					sprintf(size, "%d", (int)strlen(cur->data.f_name));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send filename length");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}
					if (write(net_sock, cur->data.f_name, strlen(cur->data.f_name)) < 0) {
						LOG(WARNING, "Unable to send filename");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}


					sprintf(size, "%d", (int)strlen(cur->data.o_path));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send path length");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}
					if (write(net_sock, cur->data.o_path, strlen(cur->data.o_path)) < 0) {
						LOG(WARNING, "Unable to send path");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}

					char *tmp = malloc(50);
					sprintf(tmp, "%li", (long int)cur->data.d_begin);
					sprintf(size, "%d", (int)strlen(tmp));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send begin date length");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}
					if (write(net_sock, tmp, strlen(tmp)) < 0) {
						LOG(WARNING, "Unable to send begin date");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}

					sprintf(tmp, "%li", (long int)cur->data.d_expire);
					sprintf(size, "%d", (int)strlen(tmp));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send expire date length");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}
					if (write(net_sock, tmp, strlen(tmp)) < 0) {
						LOG(WARNING, "Unable to send expire date");
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
					}
					free(tmp);
				}

				if (write(net_sock, "EOF", 4) < 0) {
					LOG(WARNING, "Unable to send EOF");
				}
				

				goto out;
			} else {
				LIST_FOREACH(&qr_list, first, next, cur) {
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
			clear_qr_list(&qr_list);
			free(list_path);
			break;
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

int _execute_scan_action(const char *buf, const int c_len, const int action, const int net_sock)
{
	int len, s_cl, fd, i;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;
	TWatchElement new_elem;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		LOG(URGENT, "Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, SCAN_SOCK, strlen(SCAN_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		LOG(URGENT, "Unable to connect the qr_sock");
		return -1;
	}
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if ((query == NULL) || (res == NULL)) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	switch (action) {
		case SCAN_ADD:
			if (access(buf, F_OK) == -1) {
				write_to_log(WARNING, "%s:%d: Unable to locate %s",
					__func__, __LINE__, buf);
				goto error;
			}
			strcpy(new_elem.path, buf);

			unsigned char options_unsigned[10];
			if (read(net_sock, options_unsigned, 10) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read options from socket", 
					__func__, __LINE__);
				goto error;
			}

			for (i = 0; i < 10; i++) {
				new_elem.options[i] = options_unsigned[i];
			}
			new_elem.options[SCAN_OPT_END] = '\0';
			SOCK_ANS(net_sock, SOCK_ACK);

			/* -------------------- back_limit_size reception --------------- */

			unsigned char len_unsigned[255];
			int param_len;
			if (read(net_sock, len_unsigned, 255) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				goto error;
			}
			SOCK_ANS(net_sock, SOCK_ACK);
			param_len = 0;
			for (i = 0; i < 255; i++) {
				if (len_unsigned[i] == (char) 0) 
					break;
				param_len += (int)len_unsigned[i];
			}
			unsigned char *tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read back_limit_size from socket",
					__func__, __LINE__);
				goto error;				
			}
			char *tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			sscanf(tmp_buf, "%lf", &new_elem.back_limit_size);
			
			SOCK_ANS(net_sock, SOCK_ACK);

			/* ------------------- del_limit_size reception ----------------------------- */

			if (read(net_sock, len_unsigned, 255) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				goto error;
			}
			SOCK_ANS(net_sock, SOCK_ACK);
			param_len = 0;
			for (i = 0; i < 255; i++) {
				if (len_unsigned[i] == (char) 0) 
					break;
				param_len += (int)len_unsigned[i];
			}
			
			tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read del_limit_size from socket",
					__func__, __LINE__);
				goto error;				
			}
			tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			sscanf(tmp_buf, "%lf", &new_elem.del_limit_size);
			
			SOCK_ANS(net_sock, SOCK_ACK);

			/* ----------------------- isTemp reception ----------------------------- */

			unsigned char tmpBool;
			if (read(net_sock, &tmpBool, 1) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read back_limit_size from socket",
					__func__, __LINE__);
				goto error;				
			}
			if (tmpBool == '1') new_elem.isTemp = true;
			else new_elem.isTemp = false;
			SOCK_ANS(net_sock, SOCK_ACK);

			/* ----------------------- max_age reception --------------------------- */
			
			if (read(net_sock, len_unsigned, 255) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				goto error;
			}
			SOCK_ANS(net_sock, SOCK_ACK);
			param_len = 0;
			for (i = 0; i < 255; i++) {
				if (len_unsigned[i] == (char) 0) 
					break;
				param_len += (int)len_unsigned[i];
			}
			
			tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read max_age from socket",
					__func__, __LINE__);
				goto error;				
			}
			tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			sscanf(tmp_buf, "%d", &new_elem.max_age);
			
			SOCK_ANS(net_sock, SOCK_ACK);

			free(tmp_buf_uns);
			free(tmp_buf);

			time_t timestamp = time(NULL);
			char *tmp_filename = malloc(sizeof(char)*(sizeof(timestamp)+5+strlen(SCAN_TMP)));
			if (!tmp_filename) {
				LOG(FATAL, "Unable to allocate memory");
				return -1;
			}
			if (sprintf(tmp_filename, "%s/%d", SCAN_TMP, (int)timestamp) < 0) {
				LOG(URGENT, "Unable to create the filename for temp scan file");
				free(tmp_filename);
				return -1;
			}
			fd = open(tmp_filename, O_WRONLY | O_TRUNC | O_CREAT);
			if (fd < 0) {
				write_to_log(URGENT, "%s:%d: Unable to create %s",
					__func__, __LINE__, tmp_filename);
				free(tmp_filename);
				return -1;
			}

			if (write(fd, &new_elem, sizeof(struct watchElement)) < 0) {
				write_to_log(URGENT, "%s:%d: Unable to write the new item to %s", 
					__func__, __LINE__, tmp_filename);
				free(tmp_filename);
				return -1;
			}
			close(fd);
			int path_len = strlen(tmp_filename)+1;
			snprintf(query, len, "%d:%d", action, path_len);
			if (write(s_cl, query, len) < 0) {
				LOG(URGENT, "Unable to send query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename);
				goto error;
			}
			
			if (write(s_cl, tmp_filename, path_len) < 0) {
				LOG(URGENT, "Unable to send args of the query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename);
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename);
				goto error;
			}

			if (!strcmp(res, SOCK_ACK)) {
				write_to_log(INFO, "%s has been successfully added to Scanner\n", buf);
				SOCK_ANS(net_sock, SOCK_ACK);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				write_to_log(INFO, "%s:%d: An error occured while adding %s to Scanner\n",
				__func__, __LINE__, buf);
				SOCK_ANS(net_sock, SOCK_ABORTED);
			}
			free(tmp_filename);

			break;
		case SCAN_RM:
			snprintf(query, len, "%d:%d", action, c_len);
			if (write(s_cl, query, len) < 0) {
				LOG(URGENT, "Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;
			}
			
			if (write(s_cl, buf, c_len) < 0) {
				LOG(URGENT, "Unable to send args of the query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;
			}
			if (!strcmp(res, SOCK_ACK)) {
				write_to_log(INFO, "%s has been successfully removed from Scanner\n", buf);
				SOCK_ANS(net_sock, SOCK_ACK);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				write_to_log(INFO, "%s:%d: An error occured while removing %s to Scanner\n",
				__func__, __LINE__, buf);
				SOCK_ANS(net_sock, SOCK_ABORTED);
			}
			break;
		case SCAN_LIST:
			snprintf(query, len, "%d:0", action);			
			char *list_path = malloc(PATH_MAX);
			TWatchElementList *scan_list = NULL;
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
				perror("[UI] Action get-scan-list couldn't be executed");
				free(list_path);
				goto error;
			} 

			fd = open(list_path, O_RDONLY, S_IRUSR);
			if (fd < 0) {
				perror("[UI] Unable to open scan list file");
				free(list_path);
				goto error;
			}
			load_tmp_scan(&scan_list, fd);
			close(fd);
			if (unlink(list_path))
				printf("Unable to remove temporary scan list file: %s", list_path);


			SCAN_LIST_FOREACH(scan_list, first, next, cur) {
				char size[20];
				sprintf(size, "%d", (int)strlen(cur->element.path));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send path length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				if (write(net_sock, cur->element.path, strlen(cur->element.path)) < 0) {
					LOG(WARNING, "Unable to send path");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}


				sprintf(size, "%d", (int)strlen(cur->element.options));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send options length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}				
				if (write(net_sock, cur->element.options, strlen(cur->element.options)) < 0) {
					LOG(WARNING, "Unable to send options");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				char *tmp = malloc(50);
				sprintf(tmp, "%.2lf", cur->element.back_limit_size);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					char *tmp = malloc(50);
				sprintf(tmp, "%.2lf", cur->element.back_limit_size);LOG(WARNING, "Unable to send back_limit_size length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send back_limit_size");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}

				sprintf(tmp, "%.2lf", cur->element.del_limit_size);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send del_limit_size length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send del_limit_size");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				free(tmp);
				tmp = malloc(2);
				sprintf(tmp, "%d", (cur->element.isTemp) ? 1 : 0);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send isTemp length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send isTemp");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}

				free(tmp);
				tmp = malloc(50);
				sprintf(tmp, "%d", cur->element.max_age);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send max_age length");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send max_age");
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
				}				
				free(tmp);
			}

			if (write(net_sock, "EOF", 4) < 0) {
				LOG(WARNING, "Unable to send EOF");
			}

			break;
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

int _execute_monitor_action(const char *buf, const int c_len, const int action, const int net_sock) 
{
	NOT_YET_IMP;
	return 0;
}


int execute_action(const char *buf, const int c_len, const int action, const int s_cl)
{
	switch (action) {
		case QR_ADD:
		case QR_RM:
		case QR_REST:
		case QR_LIST:
		case QR_INFO:
		case QR_RM_ALL:
		case QR_REST_ALL:
			if (_execute_qr_action(buf, c_len, action, s_cl) < 0) {
				return -1;
			}
			break;
		case SCAN_ADD:
	 	case SCAN_RM:
		case SCAN_LIST:
			if (_execute_scan_action(buf, c_len, action, s_cl) < 0) {
				return -1;
			}
			break;
		case NET_MONITOR:
			if (_execute_monitor_action(buf, c_len, action, s_cl) < 0) {
				return -1;
			}
			break;
		case KL_EXIT:
			break;
		default:
			LOG(WARNING, "Unknow command received");
	}

	return 0;
}