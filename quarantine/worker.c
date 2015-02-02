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
 * Return number of char read if >= 0, else -1
 */
int _get_data(const int sock, int *action, char **buf)
{
	int c_len = PATH_MAX + 5;
	char a_type[c_len];
	int len;
	
	if (recv(sock, a_type, c_len, 0) < 0) {
		perror("QR-WORKER: Error while receiving data through socket");
		return -1;
	}

	if (SOCK_ANS(sock, SOCK_ACK) < 0) {
		perror("QR-WORKER: Unable to send ack in socket");
		return -1;
	}

	*action = atoi(strtok(a_type, ":"));
	len = atoi(strtok(NULL, ":"));
	
	if (len > 0) {
		*buf = malloc(sizeof(char)*len);
		if (*buf == NULL) {
			if (SOCK_ANS(sock, SOCK_RETRY) < 0)
				perror("QR-WORKER: Unable to send retry");
			perror("QR-WORKER: Unable to allocate memory");
			return -1;
		}

		if (recv(sock, *buf, len, 0) < 0) {
			if (SOCK_ANS(sock, SOCK_NACK) < 0)
				perror("QR-WORKER: Unable to send nack");
			perror("QR-WORKER: Unable to allocate memory");
			return -1;			
		}

		if(SOCK_ANS(sock, SOCK_ACK) < 0) {
			perror("[QR-WORKER] Unable to send ack");
			return -1;
		}
		printf("je suis dans %s\n", __func__);
	}
	return len;
}

/* Call the action related to the "action" arg 
 * If action has been executed correctly return 0, 
 * if not return -1
 */
int _call_related_action(const int action, char *buf, const int s_cl) 
{
	static QrSearchTree list = NULL;
	QrSearchTree tmpList = NULL;
	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_worker < 0) {
		perror("QR-WORKER: Unable to create the sema to sync");
		return -1;
	}

	if (_check_qr_db() == 1) {
		wait_crit_area(sync_worker, 0);
		
		sem_down(sync_worker, 0);
		list = load_qr();
		sem_up(sync_worker, 0);
	} 
	
	switch (action) {
		case QR_ADD: 
			wait_crit_area(sync_worker, 0);
			sem_down(sync_worker, 0);
			if ((tmpList = add_file_to_qr(list, buf)) == NULL) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					perror("QR-WORKER: Unable to send aborted");
				return -1;
			}
			sem_up(sync_worker, 0);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_RM:
			wait_crit_area(sync_worker, 0);
			sem_down(sync_worker, 0); 
			DEBUG_NOTIF;
			printf("%s: I received this: %s\n", __func__, buf);
			if ((tmpList = rm_file_from_qr(list, buf)) == NULL) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					perror("QR-WORKER: Unable to send aborted");
				return -1;
			}
			sem_up(sync_worker, 0);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_REST:
			wait_crit_area(sync_worker, 0);
			sem_down(sync_worker, 0);		
			if ((tmpList = restore_file(list, buf)) == NULL) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					perror("QR-WORKER: Unable to send aborted");
				return -1;
			}
			sem_up(sync_worker, 0);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case QR_LIST:
			NOT_YET_IMP;
			break;
		case QR_INFO:
			NOT_YET_IMP;
			break;
		case QR_EXIT:
			wait_crit_area(sync_worker, 0);
			sem_down(sync_worker, 0);		
			if (save_qr_list(list)) {
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					perror("QR-WORKER: Unable to send aborted");
				return -1;
			}
			sem_up(sync_worker, 0);
			SOCK_ANS(s_cl, SOCK_ACK);
			free(list);
			break;
		default:
			return -1;
	} 
	if (tmpList != NULL) list = tmpList;
	return 0;
}

/* Get instructions via Unix domain sockets
 * and execute actions accordingly
 * 
 * For client side, follow these steps:
 * - Connect to the socket
 * - Send the action to execute (see quarantine.h)
 * with the length of second arg (format is action_num:length_sec_arg)
 * - Wait for SOCK_ARG
 * - Send the second arg (if applicable)
 * - Wait for action result
 */
void _get_instructions()
{
	int len, s_srv, s_cl;
	int action = -1;
	struct sockaddr_un server;

	if ((s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("QR-WORKER: Unable to open the socket");
		return;
	}
	printf("je suis dans %s\n", __func__);
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
		char *buf = NULL;

		len = sizeof(remote);
		if ((s_cl = accept(s_srv, (struct sockaddr *)&remote, (socklen_t *)&len)) == -1) {
			perror("QR-WORKER: Unable to accept the connection");
			continue; /*----------- TO TEST ----------------*/
		}

		if (_get_data(s_cl, &action, &buf) < 0) continue;
		if (_call_related_action(action, buf, s_cl) == -1) {
			perror("QR-WORKER: Unable to execute action");
		}

		free(buf);
		close(s_cl);
	} while(action != 0);
}

/*
 * Searches QR list and deletes a file who's date is older than todays date time
 * Calls to rm_file_from_qr to delete file physically, logically
 */
QrSearchTree _search_expired(QrSearchTree list, int *removed, time_t now)
{
	if (list == NULL)
      		return NULL;
      	if (list->left != NULL)
      		_search_expired(list->left, removed, now);
	if (list->right != NULL)
		_search_expired(list->right, removed, now);
      	if (list->data.d_expire < now) {
		list = rm_file_from_qr(list, list->data.f_name);
		*removed += 1;
      	}
	return list;
}

/*
 * Function of process who is tasked with deleting files
 * earmarked by a deletion date older than todays date time
 * Loops until no more expired files are detected
 */
void _expired_files()
{
	int removed;
	static QrSearchTree list;
	time_t now;
	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_worker < 0) {
		perror("QR-WORKER: Unable to create the sema to sync");
		return;
	}
	sem_reset(sync_worker, 0);
	do {
		removed = 0;
		now = time(NULL);
		wait_crit_area(sync_worker, 0);
		sem_down(sync_worker, 0);
		list = load_qr();
		sem_up(sync_worker, 0);
		/*
		 * TO IMPLEMENT: PROCESS IN CASE OF BELOW ERROR
		 */
		if (list == NULL) {
			if (access(QR_DB, F_OK) == -1) {
				perror("QR-WORKER: QR FILE DOES NOT EXIST");
				usleep(300000);
				_expired_files();
			} else {
				perror("QR-WORKER: QR file exists, but cannot be loaded -- process ending");
				exit(EXIT_FAILURE);
			}
		}
		if ((list = _search_expired(list, &removed, now)) != NULL) {
			wait_crit_area(sync_worker, 0);
			sem_down(sync_worker, 0);
			if (save_qr_list(list) != 0)
				perror("QR-WORKER: QR file could not be saved");
			sem_up(sync_worker, 0);
		}
	} while ( removed != 0 );
	free(list);
	return;
}

/* Main function of qr-worker process */
void qr_worker()
{
	int pid = fork();
	if (pid) {
		_get_instructions();
	} else {
		while(1) {
			_expired_files();
			usleep(300000);
		}
		
	}
}