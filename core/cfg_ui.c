/*
 * Contains all routines to execute actions linked to Config
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
 #include <global.h>
 #include <core/ui.h>
 #include <config/config.h>

 int cfg_query(int nb, char **commands, int action)
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
	strncpy(remote.sun_path, CFG_SOCK, strlen(CFG_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		perror("[UI] Unable to connect the cfg_sock");
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
		default: fprintf(stderr, "[UI] Unknown Action");
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