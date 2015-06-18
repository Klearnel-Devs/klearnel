 /**
 * \file networker.c
 * \brief Network features file
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
#include <config/config.h>
#include <net/network.h>



/*-------------------------------------------------------------------------*/
/**
 \brief Execute the Quarantine action
 \param buf the buffer received through network socket
 \param c_len the length of the command
 \param action the action to execute
 \param net_sock the client socket to send result or receive further information
 \return Returns 0 on success or -1 on error
*/
/*-------------------------------------------------------------------------*/
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
		goto error;
	}
	switch (action) {
		case QR_ADD:
			if (access(buf, F_OK) == -1) {
				write_to_log(WARNING, "%s: Unable to find %s.\n"
					"Please check if the path is correct.\n", __func__, buf);
				sprintf(query, "%s", VOID_LIST);
				if (write(s_cl, query, strlen(query)) < 0) {
					LOG(URGENT, "Unable to send file location state");
					goto error;
				}
				if (read(s_cl, res, 1) < 0) {
					LOG(WARNING, "Unable to get location result");
				}
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
				LOG(FATAL, "Unable to allocate memory");
				goto error;
			}
			if (write(s_cl, query, len) < 0) {
				LOG(WARNING, "Unable to send query");
				free(list_path);
				goto error;
			} 
			if (read(s_cl, res, 1) < 0) {
				LOG(WARNING, "Unable to get query result");
				goto error;
			}
			if (read(s_cl, list_path, PATH_MAX) < 0) {
				LOG(WARNING, "Unable to get query result");
				free(list_path);
				goto error;	
			} 
			if (strncmp(list_path, VOID_LIST, strlen(VOID_LIST)) == 0) {
				if (write(net_sock, VOID_LIST, strlen(VOID_LIST)) < 0) {
					LOG(WARNING, "Unable to send VOID_LIST through network");
					goto error;
				}
				goto out;
			}
			if (strcmp(list_path, SOCK_ABORTED) == 0) {
				LOG(WARNING, "Action get-qr-list couldn't be executed");
				free(list_path);
				goto error;
			} 

			fd = open(list_path, O_RDONLY, S_IRUSR);
			if (fd < 0) {
				LOG(WARNING, "Unable to open qr list file");
				free(list_path);
				goto error;
			}
			load_tmp_qr(&qr_list, fd);
			close(fd);
			if (unlink(list_path))
				printf("Unable to remove temporary quarantine file: %s", list_path);
			if (action == QR_LIST) {
				TMP_LIST_FOREACH(&qr_list, first, next, cur) {
					char size[20];
					sprintf(size, "%d", (int)strlen(cur->data.f_name));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send filename length");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}
					if (write(net_sock, cur->data.f_name, strlen(cur->data.f_name)) < 0) {
						LOG(WARNING, "Unable to send filename");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}


					sprintf(size, "%d", (int)strlen(cur->data.o_path));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send path length");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}
					if (write(net_sock, cur->data.o_path, strlen(cur->data.o_path)) < 0) {
						LOG(WARNING, "Unable to send path");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}

					char *tmp = malloc(50);
					sprintf(tmp, "%li", (long int)cur->data.d_begin);
					sprintf(size, "%d", (int)strlen(tmp));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send begin date length");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}
					if (write(net_sock, tmp, strlen(tmp)) < 0) {
						LOG(WARNING, "Unable to send begin date");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}

					sprintf(tmp, "%li", (long int)cur->data.d_expire);
					sprintf(size, "%d", (int)strlen(tmp));
					if (write(net_sock, size, 20) < 0) {
						LOG(WARNING, "Unable to send expire date length");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}
					if (write(net_sock, tmp, strlen(tmp)) < 0) {
						LOG(WARNING, "Unable to send expire date");
						goto error;
					}
					if (read(net_sock, res, 1) < 0) {
						LOG(WARNING, "Unable to read ACK");
						goto error;
					}
					free(tmp);
				}

				if (write(net_sock, "EOF", 4) < 0) {
					LOG(WARNING, "Unable to send EOF");
				}
				

				goto out;
			} else {
				TMP_LIST_FOREACH(&qr_list, first, next, cur) {
					char *next_query = malloc(len);
					if (next_query == NULL) {
						LOG(FATAL, "Unable to allocate memory");
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
						LOG(WARNING, "Unable to send query");
						goto error;
					}
					if (read(s_cl, res, 1) < 0) {
						LOG(WARNING, "Unable to get query result");
						goto error;
					}
					if (write(s_cl, cur->data.f_name, c_len) < 0) {
						LOG(WARNING, "Unable to send args of the query");
						goto error;
					}
					if (read(s_cl, res, 1) < 0) {
						LOG(WARNING, "Unable to get query result");
						goto error;					
					}
					if (read(s_cl, res, 1) < 0) {
						LOG(WARNING, "Unable to get query result");
						goto error;
					}
					if (!strcmp(res, SOCK_ACK)) {
						switch (action) {
							case QR_RM_ALL: write_to_log(INFO, "File %s successfully deleted from QR\n", cur->data.f_name); break;
							case QR_REST_ALL: write_to_log(INFO, "File %s successfully restored\n", cur->data.f_name); break;
						}
					} else if (!strcmp(res, SOCK_ABORTED)) {
						switch (action) {
							case QR_RM_ALL: write_to_log(INFO, "File %s could not be deleted from QR\n", cur->data.f_name); break;
							case QR_REST_ALL: write_to_log(INFO, "File %s could not be restored\n", cur->data.f_name); break;
						}
					} else if (!strcmp(res, SOCK_UNK)) {
						write_to_log(WARNING, "File not found in quarantine\n");
					}
					free(next_query);
				}
			}
out:
			clear_tmp_qr_list(&qr_list);
			free(list_path);
			free(qr_list);
			break;
		case RELOAD_CONF:
			snprintf(query, len, "%d:0", RELOAD_CONF);
			if (write(s_cl, query, len) < 0) {
				LOG(WARNING, "Unable to send query");
				goto error;
			} 
			if (read(s_cl, res, 1) < 0) {
				LOG(WARNING, "Unable to get query result");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(WARNING, "Unable to get command result");
				goto error;
			}
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

/*-------------------------------------------------------------------------*/
/**
 \brief Execute the Scanner action
 \param buf the buffer received through network socket
 \param c_len the length of the command
 \param action the action to execute
 \param net_sock the client socket to send result or receive further information
 \return Returns 0 on success or -1 on error
*/
/*-------------------------------------------------------------------------*/
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
		goto error;
	}
	switch (action) {
		case SCAN_ADD:
			if (access(buf, F_OK) == -1) {
				write_to_log(WARNING, "%s:%d: Unable to locate %s",
					__func__, __LINE__, buf);
				sprintf(query, "%s", VOID_LIST);
				if (write(s_cl, query, strlen(query)) < 0) {
					LOG(URGENT, "Unable to send file location state");
					goto error;
				}
				if (read(s_cl, res, 1) < 0) {
					LOG(WARNING, "Unable to get location result");
				}
				goto error;
			}
			
			strcpy(new_elem.path, buf);

			unsigned char options_unsigned[OPTIONS - 1];
			if (read(net_sock, options_unsigned, OPTIONS - 1) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read options from socket", 
					__func__, __LINE__);
				goto error;
			}

			for (i = 0; i < 10; i++) {
				new_elem.options[i] = options_unsigned[i];
			}
			new_elem.options[SCAN_OPT_END] = '\0';
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto error;
			}

			/* -------------------- back_limit_size reception --------------- */

			char len_unsigned[20];
			char *end;
			int param_len;
			if (read(net_sock, len_unsigned, 20) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				goto error;
			}
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto error;
			}
			param_len = atoi(len_unsigned);
			unsigned char *tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read back_limit_size from socket",
					__func__, __LINE__);
				free(tmp_buf_uns);
				goto error;				
			}
			char *tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = (char)tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			new_elem.back_limit_size = strtod(tmp_buf, &end);
			
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;
			}

			/* ------------------- del_limit_size reception ----------------------------- */

			if (read(net_sock, len_unsigned, 20) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				goto error;
			}
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto error;
			}
			param_len = atoi(len_unsigned);
			
			tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read del_limit_size from socket",
					__func__, __LINE__);
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;				
			}
			tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = (char)tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			new_elem.del_limit_size = strtod(tmp_buf, &end);
			
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;
			}

			/* ----------------------- isTemp reception ----------------------------- */

			unsigned char tmpBool;
			if (read(net_sock, &tmpBool, 1) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read back_limit_size from socket",
					__func__, __LINE__);
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;				
			}
			if (tmpBool == '1') new_elem.isTemp = true;
			else new_elem.isTemp = false;
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;
			}

			/* ----------------------- max_age reception --------------------------- */
			
			if (read(net_sock, len_unsigned, 20) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to get length of params from socket",
					__func__, __LINE__);
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;
			}
			SOCK_ANS(net_sock, SOCK_ACK);
			param_len = atoi(len_unsigned);
			tmp_buf_uns = malloc(sizeof(unsigned char)*param_len);
			if (read(net_sock, tmp_buf_uns, param_len) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read max_age from socket",
					__func__, __LINE__);
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;				
			}
			tmp_buf = malloc(sizeof(char)*(param_len + 1));
			for (i = 0; i < param_len; i++) {
				tmp_buf[i] = (char)tmp_buf_uns[i];
			}
			tmp_buf[param_len] = '\0';
			new_elem.max_age = strtod(tmp_buf, &end);
			
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				free(tmp_buf_uns);
				free(tmp_buf);
				goto error;
			}

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
		case SCAN_MOD:
			if (access(buf, F_OK) == -1) {
				write_to_log(WARNING, "%s:%d: Unable to locate %s",
					__func__, __LINE__, buf);
				goto error;
			}
			strcpy(new_elem.path, buf);

			unsigned char options_unsigned_mod[OPTIONS - 1];
			if (read(net_sock, options_unsigned_mod, OPTIONS - 1) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read options from socket", 
					__func__, __LINE__);
				goto error;
			}

			for (i = 0; i < 10; i++) {
				new_elem.options[i] = options_unsigned_mod[i];
			}
			new_elem.options[SCAN_OPT_END] = '\0';
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto error;
			}

			/* ----------------------- isTemp reception ----------------------------- */

			unsigned char tmpBool_mod;
			if (read(net_sock, &tmpBool_mod, 1) < 0) {
				write_to_log(WARNING, "%s:%d: Unable to read back_limit_size from socket",
					__func__, __LINE__);
				goto error;				
			}
			
			if (tmpBool_mod == '1') new_elem.isTemp = true;
			else new_elem.isTemp = false;
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto error;
			}

			new_elem.del_limit_size = 0;
			new_elem.back_limit_size = 0;
			new_elem.max_age = 0;

			time_t timestamp_mod = time(NULL);
			char *tmp_filename_mod = malloc(sizeof(char)*(sizeof(timestamp_mod)+5+strlen(SCAN_TMP)));
			if (!tmp_filename_mod) {
				LOG(FATAL, "Unable to allocate memory");
				return -1;
			}
			if (sprintf(tmp_filename_mod, "%s/%d", SCAN_TMP, (int)timestamp_mod) < 0) {
				LOG(URGENT, "Unable to create the filename for temp scan file");
				free(tmp_filename_mod);
				return -1;
			}

			fd = open(tmp_filename_mod, O_WRONLY | O_TRUNC | O_CREAT);
			if (fd < 0) {
				write_to_log(URGENT, "%s:%d: Unable to create %s",
					__func__, __LINE__, tmp_filename_mod);
				free(tmp_filename_mod);
				return -1;
			}
			if (write(fd, &new_elem, sizeof(struct watchElement)) < 0) {
				write_to_log(URGENT, "%s:%d: Unable to write the new item to %s", 
					__func__, __LINE__, tmp_filename_mod);
				free(tmp_filename_mod);
				return -1;
			}
			close(fd);
			int path_len_mod = strlen(tmp_filename_mod)+1;
			snprintf(query, len, "%d:%d", action, path_len_mod);
			if (write(s_cl, query, len) < 0) {
				LOG(URGENT, "Unable to send query");
				free(tmp_filename_mod);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename_mod);
				goto error;
			}
			
			if (write(s_cl, tmp_filename_mod, path_len_mod) < 0) {
				LOG(URGENT, "Unable to send args of the query");
				free(tmp_filename_mod);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename_mod);
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(URGENT, "Unable to get query result");
				free(tmp_filename_mod);
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
			free(tmp_filename_mod);

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
			if (strncmp(list_path, VOID_LIST, strlen(VOID_LIST)) == 0) {
				if (write(net_sock, VOID_LIST, strlen(VOID_LIST)) < 0) {
					LOG(WARNING, "Unable to send VOID_LIST through network");
					goto error;
				}
				goto out;
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
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					goto error;
				}
				if (write(net_sock, cur->element.path, strlen(cur->element.path)) < 0) {
					LOG(WARNING, "Unable to send path");
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					goto error;
				}


				sprintf(size, "%d", (int)strlen(cur->element.options));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send options length");
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					goto error;
				}				
				if (write(net_sock, cur->element.options, strlen(cur->element.options)) < 0) {
					LOG(WARNING, "Unable to send options");
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					goto error;
				}
				char *tmp = malloc(50);
				sprintf(tmp, "%.2lf", cur->element.back_limit_size);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send back_limit_size length");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send back_limit_size");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}

				sprintf(tmp, "%.2lf", cur->element.del_limit_size);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send del_limit_size length");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send del_limit_size");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}
				free(tmp);
				tmp = malloc(2);
				sprintf(tmp, "%d", (cur->element.isTemp) ? 1 : 0);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send isTemp length");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send isTemp");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}

				free(tmp);
				tmp = malloc(50);
				sprintf(tmp, "%d", cur->element.max_age);
				sprintf(size, "%d", (int)strlen(tmp));
				if (write(net_sock, size, 20) < 0) {
					LOG(WARNING, "Unable to send max_age length");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}
				if (write(net_sock, tmp, strlen(tmp)) < 0) {
					LOG(WARNING, "Unable to send max_age");
					free(tmp);
					goto error;
				}
				if (read(net_sock, res, 1) < 0) {
					LOG(WARNING, "Unable to read ACK");
					free(tmp);
					goto error;
				}				
				free(tmp);
			}

			if (write(net_sock, "EOF", 4) < 0) {
				LOG(WARNING, "Unable to send EOF");
			}

			break;
		case RELOAD_CONF:
			snprintf(query, len, "%d:0", RELOAD_CONF);
			if (write(s_cl, query, len) < 0) {
				LOG(WARNING, "Unable to send query");
				goto error;
			} 
			if (read(s_cl, res, 1) < 0) {
				LOG(WARNING, "Unable to get query result");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				LOG(WARNING, "Unable to get command result");
				goto error;
			}
	}
out:
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

/*-------------------------------------------------------------------------*/
/**
 \brief Send the configuration entry through network socket
 \param net_sock the client socket in which the entry will be sent
 \param section the section of the configuration entry to send
 \param key the key of the configuration entry to send
 \return Returns 0 on success or -1 on error
*/
/*-------------------------------------------------------------------------*/
int _send_conf_val(const int net_sock, char *section, char *key)
{
	char *value = get_cfg(section, key);
	char size[20];
	sprintf(size, "%d", (int)strlen(value));
	char ack[2];
	if (write(net_sock, size, 20) < 0) {
		write_to_log(URGENT, "Unable to send %s:%s size", section, key);
		return -1;
	}

	if (read(net_sock, ack, 1) < 0) {
		write_to_log(WARNING, "Unable to read ACK");
		return -1;
	}
	if (write(net_sock, value, strlen(value)) < 0) {
		write_to_log(URGENT, "Unable to send the value of %s:%s", section, key);
		return -1;
	}
	if (read(net_sock, ack, 1) < 0) {
		write_to_log(WARNING, "Unable to read ACK");
		return -1;
	}
	if (strncmp(ack, SOCK_ACK, 1) == 0) {
		return 0;
	} else {
		return -1;
	}
}


/*-------------------------------------------------------------------------*/
/**
 \brief Execute the Configuration action
 \param buf the buffer received through network socket
 \param c_len the length of the command
 \param action the action to execute
 \param net_sock the client socket to send result or receive further information
 \return Returns 0 on success or -1 on error
*/
/*-------------------------------------------------------------------------*/
int _execute_conf_action(const char *buf, const int c_len, const int action, const int net_sock) 
{
	switch(action) {
		case CONF_LIST: ;
			if (_send_conf_val(net_sock, "GLOBAL", "LOG_AGE") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "GLOBAL", "SMALL") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "GLOBAL", "MEDIUM") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "GLOBAL", "LARGE") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "SMALL", "EXP_DEF") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "SMALL", "BACKUP") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "SMALL", "LOCATION") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "MEDIUM", "EXP_DEF") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "MEDIUM", "BACKUP") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "MEDIUM", "LOCATION") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "LARGE", "EXP_DEF") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "LARGE", "BACKUP") < 0) {
				return -1;
			}
			if (_send_conf_val(net_sock, "LARGE", "LOCATION") < 0) {
				return -1;
			}
 			break;
		case CONF_MOD: ;
			/* ------------- Section reception ----------------- */
			char size_c[5];
			if (read(net_sock, size_c, (sizeof(char)*5)) < 0) {
				LOG(URGENT, "Unable to receive section size");
				goto err;
			}
			int size = atoi(size_c);
			SOCK_ANS(net_sock, SOCK_ACK);
			unsigned char section_u[size];
			if (read(net_sock, section_u, size) < 0) {
				LOG(URGENT, "Unable to receive section");
				goto err;
			}
			char section[size+1];
			int i;
			for (i = 0; i < size; i++) {
				section[i] = section_u[i];
			}
			section[size] = '\0';
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto err;
			}
			/* ----------------- Key reception ------------------- */
			if (read(net_sock, size_c, (sizeof(char)*20)) < 0) {
				LOG(URGENT, "Unable to receive key size");
				goto err;
			}
			size = atoi(size_c);
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto err;
			}
			unsigned char key_u[size];
			if (read(net_sock, key_u, size) < 0) {
				LOG(URGENT, "Unable to receive key");
				goto err;
			}
			char key[size+1];
			for (i = 0; i < size; i++) {
				key[i] = key_u[i];
			}
			key[size] = '\0';
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto err;
			}
			/* ----------------- Value reception ------------------- */
			if (read(net_sock, size_c, (sizeof(char)*20)) < 0) {
				LOG(URGENT, "Unable to receive value size");
				goto err;
			}
			size = atoi(size_c);
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto err;
			}
			unsigned char value_u[size];
			if (read(net_sock, value_u, size) < 0) {
				LOG(URGENT, "Unable to receive value");
				goto err;
			}
			char value[size+1];
			for (i = 0; i < size; i++) {
				value[i] = value_u[i];
			}
			value[size] = '\0';
			if (SOCK_ANS(net_sock, SOCK_ACK) < 0) {
				goto err;
			}
			/* ----------------- Apply change in configuration ----- */
			if (modify_cfg(section, key, value) < 0) {
				LOG(URGENT, "Unable to apply conf modifications");
				goto err;
			}
			save_conf();

			_execute_qr_action(buf, c_len, RELOAD_CONF, net_sock);
			_execute_scan_action(buf, c_len, RELOAD_CONF, net_sock);
			SOCK_ANS(net_sock, SOCK_ACK);
			break;
	}
	return 0;
err:	;
	int error = 0;
	socklen_t len = sizeof (error);
	int retval = getsockopt (net_sock, SOL_SOCKET, SO_ERROR, &error, &len );
	if (retval == 0) SOCK_ANS(net_sock, SOCK_ABORTED);
	return -1;
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
		case SCAN_MOD:
			if (_execute_scan_action(buf, c_len, action, s_cl) < 0) {
				return -1;
			}
			break;
		case CONF_LIST:
		case CONF_MOD:
			if (_execute_conf_action(buf, c_len, action, s_cl) < 0) {
				return -1;
			}
			break;
		case NET_CONNEC: ;
			struct sockaddr_in addr;
			socklen_t addr_len = sizeof(addr);
			if (getpeername(s_cl, (struct sockaddr *) &addr, &addr_len) < 0) {
				return -1;
			}
			write_to_log(INFO, "New connection initialized by %s", inet_ntoa(addr.sin_addr));
			break;
		case KL_EXIT:
			write_to_log(INFO, "Networker received stop command");
			break;
		default:
			LOG(WARNING, "Unknow command received");
	}

	return 0;
}