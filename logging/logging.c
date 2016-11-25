/*-------------------------------------------------------------------------*/
/**
   \file	logging.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Logging File

   TCreates and saves log file for the Klearnel program
   Logs are stored in:
   Log name format is YYmmDD, lines as HHmmSS

*/
/*--------------------------------------------------------------------------*/

#include <global.h>
#include <logging/logging.h>
#include <config/config.h>

void init_logging()
{
	key_t sync_logging_key = ftok(IPC_RAND, IPC_LOG);
	int sync_logging = semget(sync_logging_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_logging < 0) {
		perror("LOG: Unable to create the sema to sync");
	}
	sem_reset(sync_logging, 0);
}

void delete_logs()
{
	struct dirent *dp;
	DIR *dtr;

	char *dir ;
	dir = LOG_DIR ;

	if ((dtr = opendir(dir)) == NULL) {
		write_to_log(URGENT, "%s - %s", __func__, "Can't open log directory");
		return;
	}

	char filename[100];

	while ((dp = readdir(dtr)) != NULL) {
		struct stat stbuf ;
		sprintf( filename , "%s/%s",dir,dp->d_name) ;
		if ( stat(filename,&stbuf) == -1 ) {
			write_to_log(WARNING, "%s - %s - %s", __func__, "Unable to stat file", filename);
			continue ;
		} else {
			if (difftime(time(NULL), stbuf.st_atime) > atof(get_cfg("GLOBAL","LOG_AGE"))) {
				if (unlink(filename) != 0)
					write_to_log(WARNING,"%s - %s - %s", __func__, "Unable to remove file", filename);
				else
					write_to_log(INFO, "%s - %s - %s", __func__, "LOG FILE SUCCESSFULLY DELETED", filename);
			}
				
		}
	}
	return;
}

/*  */
/*-------------------------------------------------------------------------*/
/**
  \brief    Verifies if log directory exists, creates if not    
  \return   Returns 0 on success, -1 otherwise

  
 */
/*--------------------------------------------------------------------------*/
int _check_log_file()
{
	if (access(LOG_DIR, F_OK) == -1) {
		if (mkdir(LOG_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("LOG: Unable to create log directory");
			return -1;
		}
	}
	return 0;
}

/*  */
/*-------------------------------------------------------------------------*/
/**
  \brief        Translates severity level to string
  \param        level 	The level to translate
  \return       The string representation of the log level

  
 */
/*--------------------------------------------------------------------------*/
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
    	default: x = " -  [UNDEFI]  - "; break;
	}
	return x;
}

int write_to_log(int level, const char *format, ...)
{ 
	key_t sync_logging_key = ftok(IPC_RAND, IPC_LOG);
  	int sync_logging = semget(sync_logging_key, 1, IPC_CREAT | IPC_PERMS);
	if (sync_logging < 0) {
		perror("LOG: Unable to create the sema to sync");
	    return -1;
	} 
	va_list(args);
	char date[7], tm[9];
	char *msg_level = malloc(sizeof(char)*17);
	msg_level = strcpy(msg_level, _getLevel(level)); 
	// Time variables
	time_t rawtime;
  	struct tm * timeinfo;
  	time(&rawtime);
  	timeinfo = localtime(&rawtime);
  	strftime(date, sizeof(date), "%y%m%d", timeinfo);
  	strftime(tm, sizeof(tm), "%H:%M:%S", timeinfo);
  	char *logs = malloc(strlen(LOG_DIR) + strlen(date) + strlen(".log") + 1);


  	if (!msg_level || !logs) {
  		perror("LOG: Unable to allocate memory");
  		goto err;
  	}

  	if (snprintf(logs, strlen(LOG_DIR) + strlen(date) + strlen(".log") + 1, "%s%s%s", LOG_DIR, date,".log") < 0){
  		perror("LOG: Unable print path for logs");
		goto err;
  	} 
  	if (_check_log_file() != 0) {
		perror("LOG: Error when checking log file");
		goto err;
	} 
	int old_mask = umask(0);
	wait_crit_area(sync_logging, 0);
	sem_down(sync_logging, 0);
	FILE *fd;
	/* Open a file descriptor to the file.  */
	if ((fd = fopen(logs, "a+")) == NULL) {
		perror("LOG: Unable to open/create");
		sem_reset(sync_logging, 0);
		goto err;
	}
	/* Write to file */
	fprintf(fd, "%s", tm);
	fprintf(fd, "%s", msg_level);
	va_start(args, format);
	vfprintf(fd,format,args);
	va_end(args);
	fprintf(fd, "\n"); 
	// Free memory, close files, reset sema
	fclose(fd);
	sem_reset(sync_logging, 0);
	umask(old_mask);
	free(logs);
	free(msg_level);
  	return 0;
err:
	free(msg_level);
	free(logs);
	return -1;
}
