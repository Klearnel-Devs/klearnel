/*
 * Creates and saves log file for the Klearnel program
 * Logs are stored in:
 * Log name format is YYmmDD, lines as HHmmSS
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <logging/logging.h>

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
		write_to_log(WARNING, "LOG: Can't open directory");
		return -1;
	}

	char filename[100];

	while ((dp = readdir(dtr)) != NULL) {
		struct stat stbuf ;
		sprintf( filename , "%s/%s",dir,dp->d_name) ;
		if ( stat(filename,&stbuf) == -1 ) {
			write_to_log(WARNING, "LOG: Unable to stat file");
			continue ;
		} else {
			if (difftime(time(NULL), stbuf.st_atime) > OLD) {
				if (unlink(filename) != 0)
					write_to_log(WARNING,"LOG: Unable to remove file");
				else
					write_to_log(INFO,"LOG FILE SUCCESSFULLY DELETED");
			}
				
		}
	}
	return 0;
}

/* Verifies if log directory exists, creates if not
 * Verifies if log file exists, creates if not
*/
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

/* Translates severity level to string
*/
char *getLevel(int level)
{
    char *x;
	switch (level) {
    	case 1:  x = " -  [INFO]    - "; break;
    	case 2:  x = " -  [NOTIF]   - "; break;
    	case 3:  x = " -  [WARNING] - "; break;
    	case 4:  x = " -  [URGENT]  - "; break;
    	case 5:  x = " -  [FATAL]   - "; break;
	}
	return x;
}

/* Function for writing messages to program log files
 * Entries are preceeded by time -> severity level -> message
 * File is locked to prevent multiple processes from writing simultaneously
 * Returns -1 in case of error, 0 if succeeded (to be removed)
*/
int write_to_log(int level, const char *format, ...)
{
	va_list(args);
	char date[7], tm[7];
	char *msg_level = malloc(sizeof(char)*17);
	msg_level = strcpy(msg_level, getLevel(level));

	time_t rawtime;
  	struct tm * timeinfo;
  	time(&rawtime);
  	timeinfo = localtime(&rawtime);
  	strftime(date, sizeof(date), "%y%m%d", timeinfo);
  	strftime(tm, sizeof(tm), "%H%M%S", timeinfo);

  	char *logs = malloc(strlen(LOG_DIR) + strlen(date) + 1);

  	if (!msg_level || !logs) {
  		perror("[LOG] Unable to allocate memory");
  		return -1;
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
	fclose(fd);
	free(logs);
	free(msg_level);
	va_end(args);
  	return 0;
err:
	free(msg_level);
	free(logs);
	return -1;
}
