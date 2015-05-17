 /*
 * This file contains all functions that are needed
 * for the network communication with the manager
 *
 * It uses the networker.c to call related
 * actions to execute.
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <openssl/sha.h>
#include <net/crypter.h>
#include <logging/logging.h>
#include <net/network.h>



int _check_token(const int s_cl) 
{
	char *token = malloc(sizeof(char)*255);
	if (!token) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}

	if (read(s_cl, token, (sizeof(char)*255)) < 0) {
		LOG(FATAL, "Unable to read the token");
		free(token);
		return -1;
	}

	SOCK_ANS(s_cl, SOCK_ACK);

	int fd = open(TOKEN_DB, O_RDONLY);
	char inner_token[255];
	if (read(fd, inner_token, 255) < 0) {
		LOG(FATAL, "Unable to read the inner token");
		free(token);
		close(fd);
		return -1;
	}
	close(fd);

	if (strcmp(token, inner_token) == 0) {
		SOCK_ANS(s_cl, SOCK_ACK);
		return 0;
	} else {
		SOCK_ANS(s_cl, SOCK_NACK);
		return -1;
	}
}

int _get_root(const int s_cl)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	if (read(s_cl, hash, SHA256_DIGEST_LENGTH) < 0) {
		LOG(FATAL, "Unable to read the hash");
		return -1;
	}
	SOCK_ANS(s_cl, SOCK_ACK);
	int result = check_hash(hash);

	if (result == 0) {
		SOCK_ANS(s_cl, SOCK_ACK);
		return 0;
	} else if (result == 1) {
		SOCK_ANS(s_cl, SOCK_NACK);
		return -1;
	} else {
		SOCK_ANS(s_cl, SOCK_ABORTED);
		return -1;
	}
}

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
		
		if (_check_token(s_cl) < 0) {
			free(buf);
			close(s_cl);
			continue;
		}

		if (_get_root(s_cl) < 0) {
			free(buf);
			close(s_cl);
			continue;
		}

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

