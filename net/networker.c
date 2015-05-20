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
			if (read(s_cl, res, 2) < 0) {
				>>>>>>> Network features: SCAN_ADD and SCAN_RM implemented & testedLOG(URGENT, "Unable to get query result");
				goto error;
			}

			if (write(s_cl, buf, c_len) < 0) {
				LOG(URGENT, "Unable to send args of the query");
				goto error;
			}
			if (read(s_cl, res, 2) < 0) {
				LOG(URGENT, "Unable to get query result");
				goto error;					
			}
			if (read(s_cl, res, 2) < 0) {
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
		perror("[UI] Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, SCAN_SOCK, strlen(SCAN_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
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
			write_to_log(DEBUG, "Len computed: %d", param_len);
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
			write_to_log(DEBUG, "Back limit size: %lf", new_elem.back_limit_size);
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
			write_to_log(DEBUG, "Len computed: %d", param_len);
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
			write_to_log(DEBUG, "Del limit size: %lf", new_elem.del_limit_size);
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
			write_to_log(DEBUG, "Len computed: %d", param_len);
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
			write_to_log(DEBUG, "Max Age: %d", new_elem.max_age);
			SOCK_ANS(net_sock, SOCK_ACK);

			write_to_log(DEBUG, "New Elem: \n"
					" - Path: %s\n"
					" - Options: %s\n"
					" - Backup Limit Size: %lf\n"
					" - Delete Limit size: %lf\n"
					" - Temp folder ?: %d\n"
					" - Max Age: %d",
					new_elem.path, new_elem.options, new_elem.back_limit_size,
					new_elem.del_limit_size, new_elem.isTemp, new_elem.max_age 
					);
			free(tmp_buf_uns);
			free(tmp_buf);

			time_t timestamp = time(NULL);
			char *tmp_filename = malloc(sizeof(char)*(sizeof(timestamp)+5+strlen(SCAN_TMP)));
			if (!tmp_filename) {
				LOG(FATAL, "Unable to allocate memory");
				return -1;
			}
			if (sprintf(tmp_filename, "%s/%d", SCAN_TMP, (int)timestamp) < 0) {
				perror("SCAN-UI: Unable to create the filename for temp scan file");
				free(tmp_filename);
				return -1;
			}
			fd = open(tmp_filename, O_WRONLY | O_TRUNC | O_CREAT);
			if (fd < 0) {
				fprintf(stderr, "SCAN-UI: Unable to create %s", tmp_filename);
				free(tmp_filename);
				return -1;
			}

			if (write(fd, &new_elem, sizeof(struct watchElement)) < 0) {
				fprintf(stderr, "SCAN-UI: Unable to write the new item to %s", tmp_filename);
				free(tmp_filename);
				return -1;
			}
			close(fd);
			int path_len = strlen(tmp_filename)+1;
			snprintf(query, len, "%d:%d", action, path_len);
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 2) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;
			}
			
			if (write(s_cl, tmp_filename, path_len) < 0) {
				perror("[UI] Unable to send args of the query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 2) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;					
			}
			if (read(s_cl, res, 2) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;
			}
			if (!strcmp(res, SOCK_ACK)) {
				printf("%s has been successfully added to Scanner\n", buf);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				printf("An error occured while adding %s to Scanner\n", buf);
			}
			free(tmp_filename);

			break;
		case SCAN_RM:
			snprintf(query, len, "%d:%d", action, c_len);
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 2) < 0) {
				perror("[UI] Unable to get query result");
				goto error;
			}
			
			if (write(s_cl, buf, c_len) < 0) {
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
			if (!strcmp(res, SOCK_ACK)) {
				printf("%s has been successfully removed from Scanner\n", buf);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				printf("An error occured while removing %s to Scanner\n", buf);
			}
			break;
		case SCAN_LIST:
			NOT_YET_IMP;
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
		default:
			LOG(WARNING, "Unknow command received");
	}

	return 0;
}