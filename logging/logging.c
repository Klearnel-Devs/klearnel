/*
 * Creates and saves log file for the Klearnel program
 * Logs are stored in:
 * Log name format is YYmmDD, lines as HHmmSS
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <logging/logging.h>

/* Initializes the logging semaphore */
 void init_logging()
 {
 	key_t sync_logging_key = ftok(IPC_RAND, IPC_LOG);
  	int sync_logging = semget(sync_logging_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_logging < 0) {
		perror("LOG: Unable to create the sema to sync");
	}
	sem_reset(sync_logging, 0);
 }

/* Function that iterates through the log files contained in the Klearnel
 * log directory defined in global.h. Each log files last accessed time is
 * compared to the current time, and if older than desired, deleted
 *
 * Variable desired age still needs to be implemented!
 */
int _delete_logs()
{
	struct dirent *dp;
	DIR *dtr;

	char *dir ;
	dir = LOG_DIR ;

	if ((dtr = opendir(dir)) == NULL) {
		write_to_log(URGENT, "%s - %s", __func__, "Can't open log directory");
		return -1;
	}

	char filename[100];

	while ((dp = readdir(dtr)) != NULL) {
		struct stat stbuf ;
		sprintf( filename , "%s/%s",dir,dp->d_name) ;
		if ( stat(filename,&stbuf) == -1 ) {
			write_to_log(WARNING, "%s - %s - %s", __func__, "Unable to stat file", filename);
			continue ;
		} else {
			if (difftime(time(NULL), stbuf.st_atime) > OLD) {
				if (unlink(filename) != 0)
					write_to_log(WARNING,"%s - %s - %s", __func__, "Unable to remove file", filename);
				else
					write_to_log(INFO, "%s - %s - %s", __func__, "LOG FILE SUCCESSFULLY DELETED", filename);
			}
				
		}
	}
	return 0;
}

/* Verifies if log directory exists, creates if not */
int _check_log_file(char *logs)
{
	if (access(LOG_DIR, F_OK) == -1) {
		if (mkdir(LOG_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("LOG: Unable to create log directory");
			return -1;
		}
	}
	return 0;
}

/* Translates severity level to string */
char *_getLevel(int level)
{
    char *x;
	switch (level) {
		case 0:  x = " -   [DBUG]   - "; break;
    	case 1:  x = " -   [INFO]   - "; break;
    	case 2:  x = " -  [NOTIFY]  - "; break;
    	case 3:  x = " -   [WARN]   - "; break;
    	case 4:  x = " -  [URGENT]  - "; break;
    	case 5:  x = " -   [FATL]   - "; break;
	}
	return x;
}

/* Function for writing messages to program log files
 * Entries are preceeded by time -> severity level -> message
 * Semaphore used to guarantee that only one process may write at a time
 * Variable arguments passed by parameter
 * Returns -1 in case of error, 0 if succeeded (to be removed)
*/
int write_to_log(int level, const char *format, ...)
{
	va_list(args);
	char date[7], tm[7];
	char *msg_level = malloc(sizeof(char)*17);
	msg_level = strcpy(msg_level, _getLevel(level));

	// Time variables
	time_t rawtime;
  	struct tm * timeinfo;
  	time(&rawtime);
  	timeinfo = localtime(&rawtime);
  	strftime(date, sizeof(date), "%y%m%d", timeinfo);
  	strftime(tm, sizeof(tm), "%H%M%S", timeinfo);

  	char *logs = malloc(strlen(LOG_DIR) + strlen(date) + 1);

  	key_t sync_logging_key = ftok(IPC_RAND, IPC_LOG);
  	int sync_logging = semget(sync_logging_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_logging < 0) {
		perror("LOG: Unable to create the sema to sync");
		goto err;
	}
  	if (!msg_level || !logs) {
  		perror("LOG: Unable to allocate memory");
  		goto err;
  	}

  	if (!strncpy(logs, LOG_DIR, strlen(LOG_DIR))) {
		perror("LOG: Unable to copy time to log message");
		goto err;
	}
  	if (!strncat(logs, date, strlen(date))) {
		perror("LOG: Unable to concatenate date in log path");
		goto err;
	}	
  	if (_check_log_file(logs) != 0) {
		perror("LOG: Error when checking log file");
		goto err;
	}
	wait_crit_area(sync_logging, 0);
	sem_down(sync_logging, 0);
	FILE *fd;
	/* Open a file descriptor to the file.  */
	if ((fd = fopen(logs, "a+")) == NULL)
		perror("LOG: Unable to open/create");

	/* Write to file */
	fprintf(fd, "%s", tm);
	fprintf(fd, "%s", msg_level);
	va_start(args, format);
	vfprintf(fd,format,args);
	fprintf(fd, "\n");

	// Free memory, close files, reset sema
	fclose(fd);
	sem_reset(sync_logging, 0);
	free(logs);
	free(msg_level);
	va_end(args);
  	return 0;
err:
	free(msg_level);
	free(logs);
	return -1;
}
