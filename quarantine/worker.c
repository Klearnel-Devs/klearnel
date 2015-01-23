/*
 * Process that associates tasks with required quarantine actions
 * Forked from Klearnel, forks to accomplish certain tasks
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>

/* Check if QR_DB has been modified 
 * Return: 
 *  1 if QR_DB has been modified
 *  0 if not
 * -1 in other cases
 */
int _check_qr_db()
{
	static time_t cur_date = 0;
	struct stat tmp;
	if (stat(QR_DB, &tmp) < 0) {
		perror("QR-WORKER: Unable to locate QR_DB");
		return -1;
	}
	if ((cur_date == 0) || (cur_date != tmp.st_mtime)) {
		cur_date = tmp.st_mtime;
		return 1;
	} 
	return 0;
}

/* Get data from socket "sock" and put it in buffer "buf"
 * Return number of char read if >= 0, -1 if not
 */
int _get_data(const int sock, char **buf)
{
	char a_type[100];
	int len;
	if (recv(s_cl, a_type, 100, 0) < 0) {
		perror("QR-WORKER: Error while receiving data through socket");
		return -1;
	}
	if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
		perror("QR-WORKER: Unable to send ack in socket");
		return -1;
	}
	action = atoi(strtok(a_type, ":"));
	len = atoi(strtok(NULL, ":"));
	
	if (len > 0) {
		buf = malloc(sizeof(char)*len);
		if (buf == NULL) {
			if (SOCK_ANS(s_cl, SOCK_RETRY) < 0)
				perror("QR-WORKER: Unable to send retry");
			perror("QR-WORKER: Unable to allocate memory");
			return -1;
		}
		if(recv(s_cl, buf, len, 0) < 0) && {
			if (SOCK_ANS(s_cl, SOCK_NACK) < 0)
				perror("QR-WORKER: Unable to send nack");
			perror("QR-WORKER: Unable to allocate memory");
			return -1;			
		}
	}
	return len;
}

/* Call the action related to the "action" arg 
 * If action has been executed correctly return 0, 
 * if not return -1
 */
int _call_related_action(const int action, const char *buf) 
{
	static QrSearchTree list;
	if(_check_qr_db() == 1) list = load_qr();

	switch (action) {
		QR_ADD: 
			if ((list = add_file_to_qr(list, buf) == NULL)
				return -1;
		QR_RM:
		QR_REST:
		QR_EXIT:
		default:
			return -1;
	} 
	return 0;
}

/* Get instructions via Unix domain sockets
 * and execute actions accordingly
 */
void _get_instructions()
{
	int len, s_srv, s_cl;
	int action = -1;
	struct sockaddr_un server;

	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS); 
	if (sync_worker < 0) {
		perror("QR-WORKER: Unable to create the sema to sync");
		return;
	}
	if (s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("QR-WORKER: Unable to open the socket");
		return;
	}
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, QR_SOCK, strlen(QR_SOCK) + 1);
	unlink(server.sun_path);
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if(bind(s_srv, (struct sockaddr *)&server, len) < 0) {
		perror("QR-WORKER: Unable to bind the socket");
		return;
	}
	listen(s_srv, 10);

	do {
		struct sockaddr_un remote;
		int result = -1;
		char *buf = NULL;

		len = sizeof(struct sockaddr_un);
		if ((s_cl = accept(s_srv, &remote, &len)) == -1) {
			perror("QR-WORKER: Unable to accept the connection");
			continue; /*----------- TO TEST ----------------*/
		}

		if ((len = _get_data(s_cl, &buf)) < 0) continue;
		if (_call_related_action(action, buf) == -1) {
			if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
				perror("QR-WORKER: Unable to send aborted");
			perror("QR-WORKER: Unable to execute action");
		}

		free(buf);
	} while(action != 0);
}

/*
 * Searches QR list and returns a file ready for deletion
 */
void _search_expired()
{

}

/*
 * Function to verify deletion date of files contained in QR_DB
 * Calls to rm_file_from_qr to delete file physically, logically
 */
void _expired_files()
{
	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_worker < 0) {
		perror("QR: Unable to create the sema to sync");
		return;
	}
}

void qr_worker()
{
	int pid = fork();
	if (pid) {
		_get_instructions();
	} else {
		_expired_files();
	}
}