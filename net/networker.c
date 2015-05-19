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
				switch(action) {
					case QR_ADD:  printf("File %s successfully added to QR\n", buf); break;
					case QR_RM:   printf("File %s successfully deleted from QR\n", buf); break;
					case QR_REST: printf("File %s successfully restored\n", buf); break;
				}
			} else if (!strcmp(res, SOCK_ABORTED)) {
				switch(action) {
					case QR_ADD:  printf("File %s could not be added to QR\n", buf); break;
					case QR_RM:   printf("File %s could not be deleted from QR\n", buf); break;
					case QR_REST: printf("File %s could not be restored\n", buf); break;
				}
			} else if (!strcmp(res, SOCK_UNK)) {
				fprintf(stderr, "File not found in quarantine\n");
			}		
			break;

	} 

	return 0;

error:
	free(query);
	free(res);
	close(s_cl);
	return -1;
}

int _execute_scan_action(const char *buf, const int c_len, const int action, const int net_sock)
{
	NOT_YET_IMP;
	return 0;
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