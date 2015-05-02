 /*
 * This file contains all functions that are needed
 * for the network communication with the manager
 *
 * It uses the sender.c and receiver.c to call related
 * actions to execute.
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <net/network.h>
#include <logging/logging.h>

void networker()
{
	int len, s_srv, s_cl;
	int c_len = 20;
	int action = 0;
	struct sockaddr_in server;
	int oldmask = umask(0);
	if ((s_srv = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open the socket");
		return;
	}
	bzero((char*) &server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = SOCK_NET_PORT;
	server.sin_addr.s_addr = INADDR_ANY;
	if(bind(s_srv, (struct sockaddr *)&server, sizeof(server)) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to bind the socket");
		return;
	}
	umask(oldmask);
	listen(s_srv, 10);

	do {
		struct timeval to_socket;
		to_socket.tv_sec 	= SOCK_TO;
		to_socket.tv_usec = 0;

		struct sockaddr_in remote;
		char *buf = NULL;

		len = sizeof(remote);
		if ((s_cl = accept(s_srv, (struct sockaddr *)&remote, (socklen_t *)&len)) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to accept the connection");
			continue;
		}

		if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to set timeout for reception operations");

		if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to set timeout for sending operations");
		
		if (get_data(s_cl, &action, &buf, c_len) < 0) {
			free(buf);
			close(s_cl);
			write_to_log(NOTIFY, "%s - %d - %s", __func__, __LINE__, "Unable to get action to execute");
			continue;
		}
		if (execute_action(buf, action, s_cl) < 0) {
			free(buf);
			SOCK_ANS(s_cl, SOCK_ABORTED);
			close(s_cl);
			write_to_log(NOTIFY, "%s - %d - %s %d", __func__, __LINE__, "Unable to execute the received action:", action);
			continue;
		}
		free(buf);
		close(s_cl);
	} while (action != KL_EXIT);
	close(s_srv);
}
