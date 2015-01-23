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
		perror("QR: Unable to locate QR_DB");
		return -1;
	}
	if ((cur_date == 0) || (cur_date != tmp.st_mtime)) {
		cur_date = tmp.st_mtime;
		return 1;
	} 
	return 0;
}

/* Get instructions via Unix domain sockets
 * and execute actions accordingly
 * 
 */
void _get_instructions()
{
	int action, fd;
	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS); 
	if (sync_worker < 0) {
		perror("QR: Unable to create the sema to sync");
		return;
	}

	do {

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