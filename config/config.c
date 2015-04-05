 /*
 * Contains all functions required to maintain 
 configuration profiles
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <config/config.h>
#include <logging/logging.h>

int _cfg_task(const int action, char *buf, const int s_cl) 
{
	return 0;
}

void init_config()
{
	if (access(PROFILES, F_OK) == -1) {
		if (mkdir(PROFILES, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the stock", PROFILES);
			return;
		}
	}
	if (access(CFG_TMP, F_OK) == -1) {
		if (creat(CFG_TMP, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the database file", CFG_TMP);
			return;
		}
	}
	write_to_log(INFO, "%s", "Configuration Initialized without error");
}

void cfg_worker()
{
	int len, s_srv, s_cl;
	// CHECK WITH ANTOINE
	int c_len = 20;
	int task = 0;
	struct sockaddr_un server;

	if ((s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		write_to_log(WARNING, "%s:%d: %s", __func__, __LINE__, "Unable to open the socket");
		return;
	}
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, SCAN_SOCK, strlen(SCAN_SOCK) + 1);
	unlink(server.sun_path);
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if(bind(s_srv, (struct sockaddr *)&server, len) < 0) {
		write_to_log(WARNING, "%s:%d: %s", __func__, __LINE__, "Unable to bind the socket");
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
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to accept the connection");
					continue;
				}
				if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to set timeout for reception operations");

				if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to set timeout for sending operations");		

				if (get_data(s_cl, &task, &buf, c_len) < 0) {
					free(buf);
					close(s_cl);
					write_to_log(NOTIFY, "%s:%d: %s", 
						__func__, __LINE__, "_get_data FAILED");
					continue;
				}
				_cfg_task(task, buf, s_cl);
				free(buf);
				close(s_cl);
			}
		} else {
			perform_event();
		}
	} while (task != KL_EXIT);
	close(s_srv);
	unlink(server.sun_path);
	exit_scanner();
}