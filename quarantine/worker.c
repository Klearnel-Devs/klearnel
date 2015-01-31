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

void _get_instructions()
{

}

/*
 * Searches QR list and deletes a file who's date is older than todays date time
 * Calls to rm_file_from_qr to delete file physically, logically
 */
QrSearchTree _search_expired(QrSearchTree list, int *removed, time_t now)
{
	if (list == NULL)
      		return NULL;
      	if(list->left != NULL)
      		_search_expired(list->left, removed, now);
	if(list->right != NULL)
		_search_expired(list->right, removed, now);
      	if (list->data.d_expire < now) {
		list = rm_file_from_qr(list, list->data.f_name);
		removed += 1;
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
	QrSearchTree list;
	time_t now;
	key_t sync_worker_key = ftok(IPC_RAND, IPC_QR);
	int sync_worker = semget(sync_worker_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_worker < 0) {
		perror("QR-WORKER: Unable to create the sema to sync");
		return;
	}
	do {
		removed = 0;
		now = time(NULL);
		wait_crit_area(sync_worker, 1);
		sem_down(sync_worker, 1);
		list = load_qr();
		sem_up(sync_worker, 1);
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
			wait_crit_area(sync_worker, 1);
			sem_down(sync_worker, 1);
			if (save_qr_list(list) != 0)
				perror("QR-WORKER: QR file could not be saved");
			sem_up(sync_worker, 1);
		}
	} while ( removed != 0 );
	free(list);
	return;
}

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