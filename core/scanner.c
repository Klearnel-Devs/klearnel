/*-------------------------------------------------------------------------*/
/**
   \file	scanner.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Scanner module file

   Scanner manage the automated process for folders / files it had to follow
*/
/*--------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>
#include <core/scanner.h>
#include <config/config.h>
#include <core/ui.h>

static TWatchElementList* watch_list = NULL;
static int protect_num = 4;
static int exclude_num = 2;
static const char *protect[] = {"/boot", "/proc", "/mnt", "/media"};
static const char *exclude[] = {".git", ".svn"};

/*-------------------------------------------------------------------------*/
/**
  \brief        Adds TWatchElement to temporary TWatchElemtnList
  \param        elem 	The TWatchElement to add
  \param        list 	The temporary TWatchElementList
  \return       Return 0 on success and -1 on error	

  
 */
/*--------------------------------------------------------------------------*/
int _add_tmp_watch_elem(TWatchElement elem, TWatchElementList **list) 
{
	TWatchElementNode* node = malloc(sizeof(struct watchElementNode));
	if (!node) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	node->element = elem;
	node->next = NULL;
	if (!(*list)) {
		(*list) = malloc(sizeof(struct watchElementList));
		if (!(*list)) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		(*list)->first = node;
		(*list)->last = node;
		(*list)->count = 1;
		return 0;
	}
	
	node->prev = (*list)->last;
	(*list)->last->next = node;
	(*list)->last = node;
	(*list)->count++;
	return 0;
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Backups files and folders
  \param        file_bu 	The file to backup
  \return       void	

  For files, retreives configured backup location from config file depending
  on size of file to backup. For folders, forks & execs an ls call to
  determine size of folders contents, then retrieves backup location like
  for files. Uses rsync for backup. 
 */
/*--------------------------------------------------------------------------*/
void _backupFiles(char* file_bu) 
{
	int childExitStatus;
	struct stat inode;
	pid_t pid;
	char *path, *backup_path;
	char *backup = malloc(sizeof(char)*255);
	char *file = malloc(strlen(file_bu)+1);
	char *tmp;
	char date[7];
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(date, sizeof(date), "%y", timeinfo);
	if ((backup == NULL) || (file == NULL)){
		LOG(FATAL, "Unable to allocate memory");
		return;
	}
	strcpy(file, file_bu);
	if (stat(file, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", file);
		goto err;
	}

	if (!S_ISREG(inode.st_mode) && !S_ISDIR(inode.st_mode)) {
		write_to_log(INFO, "%s - %d - %s : %s", __func__, __LINE__, 
				"Not a file or folder", file);
		goto err;
	}
	if(S_ISREG(inode.st_mode)) {
		if (inode.st_size < atoi(get_cfg("GLOBAL", "SMALL"))) {
			if (atoi(get_cfg("SMALL", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			} 
			tmp = get_cfg("SMALL", "LOCATION");
		} else if (inode.st_size > atoi(get_cfg("GLOBAL", "LARGE"))) {
			if (atoi(get_cfg("LARGE", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			}
			tmp = get_cfg("LARGE", "LOCATION");
		} else {
			if (atoi(get_cfg("MEDIUM", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			}
			tmp = get_cfg("MEDIUM", "LOCATION");
		}
		if(tmp[strlen(tmp)-1] != '/') {
			if (snprintf(backup, strlen(tmp)+strlen("/")+1, "%s%s", 
					tmp, "/") <= 0) {
				LOG(FATAL, "Unable to print path");
				goto err;
			}
		} else {
			if (snprintf(backup, strlen(tmp)+1, "%s", 
					tmp) <= 0) {
				printf("%d\n", __LINE__);
				goto err;
			}
		}
	} else {
	      	int pipe_fd[2];
	      	int size;
		if (pipe(pipe_fd) < 0) {
			write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
					"Scanner could not pipe");
			goto err;
		}

		if ( (pid = fork() ) < 0) {
			write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
					"Scanner could not fork");
			goto err;
		}

		if (pid == 0) {
		      	if (chdir(file) != 0) {
				_exit(EXIT_FAILURE);
			}
			close (pipe_fd[0]);
			if (dup2 (pipe_fd[1], 1) == -1) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
						"Failed to duplicate file descriptor");
				_exit(EXIT_FAILURE);
			}
			if (system("ls -lR | grep -v '^d' | awk '{total += $5} END {print total}'") == -1) {
				LOG(WARNING, "Failed to execute ls get dir size");
				_exit(EXIT_FAILURE);
			}
			close (pipe_fd[1]);
			exit(EXIT_SUCCESS);
		} else {
			int status;
			waitpid(pid, &status, 0);
			int i = 0;
			char buf;
		      	char *tmp2 = malloc(sizeof(char)*255);
		      	if (tmp2 == NULL) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Unable to allocate memory");
				close(pipe_fd[0]);
				close(pipe_fd[1]);
				goto err;
		      	}
			close(pipe_fd[1]);
			while (read(pipe_fd[0], &buf, 1) > 0) {
				tmp2[i] = buf;
				i++;
			}
			if ((size = atoi(tmp2)) < 0) {
				write_to_log(WARNING, "%s - %d - %s : %s", __func__, __LINE__, 
						"atoi() failed on ", tmp2);
				free(tmp2);
				close(pipe_fd[0]);
				goto err;
			}
			close(pipe_fd[0]);
			free(tmp2);
		}
		if (size < atoi(get_cfg("GLOBAL", "SMALL"))) {
			if (atoi(get_cfg("SMALL", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			} 
			tmp = get_cfg("SMALL", "LOCATION");
		} else if (size > atoi(get_cfg("GLOBAL", "LARGE"))) {
			if (atoi(get_cfg("LARGE", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			}
			tmp = get_cfg("LARGE", "LOCATION");
		} else {
			if (atoi(get_cfg("MEDIUM", "BACKUP")) == 0) {
				LOG(INFO, "No backup directory configured");
				goto err;
			}
			tmp = get_cfg("MEDIUM", "LOCATION");
		}
		if(tmp[strlen(tmp)-1] != '/') {
			if (snprintf(backup, strlen(tmp)+strlen("/")+1, "%s%s", 
					tmp, "/") <= 0) {
				LOG(FATAL, "Unable to print path");
				goto err;
			}
		} else {
			if (snprintf(backup, strlen(tmp)+1, "%s", 
					tmp) <= 0) {
				printf("%d\n", __LINE__);
				goto err;
			}
		}
		if(file[strlen(file)-1] == '/') {
			file[strlen(file)-1] = '\0';
		}
	}
	if (access(backup, F_OK) == -1) {
		write_to_log(FATAL, "Unable to access backup directory : %s", 
				backup);
		goto err;
	}
  	backup_path = malloc(strlen(backup)+strlen(date)+1);
  	if(backup_path == NULL) {
  		LOG(FATAL, "Unable to allocate memory");
		goto err;
  	}
  	if (snprintf(backup_path, strlen(backup)+strlen(date)+1, 
  		"%s%s", backup, date) <= 0) {
  		LOG(FATAL, "Unable to print path");
		goto err2;
  	}
  	if (access(backup_path, F_OK) == -1) {
		if (mkdir(backup_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			write_to_log(FATAL, "Unable to create backup subdirectory : %s", 
				backup_path);
			goto err2;
		}
	}
	if(S_ISREG(inode.st_mode)) {
		tmp = basename(file);
		char *dirname = basename(tmp);
		char *bu_path = malloc(strlen(backup_path) + strlen("/") + strlen("files/") +1);
		if (bu_path == NULL) {
			LOG(FATAL, "Unable to allocate memory");
			goto err2;
		}
		if (snprintf(bu_path, strlen(backup_path)+strlen("/")+strlen("files/"), 
	  		"%s/%s", backup_path, "files/") <= 0) {
			LOG(FATAL, "Unable to print path");
			free(bu_path);
			goto err2;
	  	}
		if (access(bu_path, F_OK) == -1) {
			if (mkdir(bu_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
				write_to_log(FATAL, "Unable to create backup subdirectory : %s", 
					backup_path);
				free(bu_path);
				goto err2;
			}
		}
		path = malloc(strlen(backup_path)+strlen("/")+strlen("files/")+strlen(dirname)+strlen("/")+
				strlen(basename(file))+1);
		if (path == NULL) {
			free(bu_path);
			LOG(FATAL, "Unable to allocate memory");
			goto err2;
		}
	  	if (snprintf(path, strlen(backup_path)+strlen("/")+strlen("files/")+strlen(dirname)+strlen("/")+1, 
	  		"%s/%s%s", backup_path, "files/",dirname) <= 0) {
			LOG(FATAL, "Unable to print path");
			free(bu_path);
			goto err3;
	  	}
	  	free(bu_path);
	} else {
		path = malloc(strlen(backup_path)+strlen("/")+1);
	  	if (path == NULL) {
			LOG(FATAL, "Unable to allocate memory");
			goto err2;
		}
	  	if (snprintf(path, strlen(backup_path)+strlen("/")+1, 
	  			"%s/", backup_path) <= 0) {
			LOG(FATAL, "Unable to print path");
			goto err3;
	  	}
	}
  	pid = fork();
	if (pid == 0) {
		char *prog1_argv[4];
		prog1_argv[0] = "rsync";
		prog1_argv[1] = "-a";
		prog1_argv[2] = file;
		prog1_argv[3] = path;
		prog1_argv[4] = NULL;
		execvp(prog1_argv[0], prog1_argv);
	} else if ( pid < 0 ) {
		LOG(FATAL, "Could not fork backup exec");
		goto err3;
	} else {
		pid_t ws = waitpid(pid, &childExitStatus, 0);
		if (ws == -1) {
	        	LOG(FATAL, "Backup Exec failed");
	        } else {
	        	LOG(INFO, "Backup performed successfully");
	        }
	}
	err3:
	free(path);
	err2:
	free(backup_path);
	err:
	free(backup);
	free(file);
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Add a file to quarantine
  \param        buf 	The file to add
  \return       Returns 0 on success, else -1

  
 */
/*--------------------------------------------------------------------------*/
int _add_file_qr(char *buf)
{
	int len, s_cl;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		LOG(FATAL, "Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, QR_SOCK, strlen(QR_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		LOG(FATAL, "Unable to connect the qr_sock");
		return -1;
	}
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		LOG(WARNING, "Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if ((query == NULL) || (res == NULL)) {
		LOG(FATAL, "Unable to allocate memory");
		goto error;
	}

	if (access(buf, F_OK) == -1) {
		write_to_log(WARNING, "%s: Unable to find %s.\n"
			"Please check if the path is correct.\n", __func__, buf);
		sprintf(query, "%s", VOID_LIST);
		if (write(s_cl, query, strlen(query)) < 0) {
			LOG(URGENT, "Unable to send file location state");
			goto error;
		}
		if (read(s_cl, res, 1) < 0) {
			LOG(WARNING, "Unable to get location result");
		}
		goto error;
	}

	snprintf(query, len, "%d:%d", QR_ADD, (int)strlen(buf)+1);
	if (write(s_cl, query, len) < 0) {
		LOG(URGENT, "Unable to send query");
		goto error;
	}
	if (read(s_cl, res, 1) < 0) {
		LOG(URGENT, "Unable to get query result");
		goto error;
	}

	if (write(s_cl, buf, strlen(buf)+1) < 0) {
		LOG(URGENT, "Unable to send args of the query");
		goto error;
	}
	if (read(s_cl, res, 1) < 0) {
		LOG(URGENT, "Unable to get query result");
		goto error;					
	}
	if (read(s_cl, res, 1) < 0) {
		LOG(URGENT, "Unable to get query result");
		goto error;
	}
	if (!strcmp(res, SOCK_ACK)) {
		write_to_log(INFO, "File %s successfully added to QR\n", buf);
	} else if (!strcmp(res, SOCK_ABORTED)) {
		write_to_log(INFO, "File %s could not be added to QR\n", buf);
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

/*-------------------------------------------------------------------------*/
/**
  \brief        Delete files and folders to Quarantine
  \param        file 	The file to delete
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _deleteFiles(char *file) 
{
	int childExitStatus, i = 0, num = 0;
	struct stat inode;
	pid_t pid;
	if (stat(file, &inode) != 0) {
		write_to_log(URGENT, "Unable to stat file : %s", file);
		goto err;
	}

	if (!S_ISREG(inode.st_mode) && !S_ISDIR(inode.st_mode)) {
		write_to_log(INFO, "%s - %d - %s : %s", __func__, __LINE__, 
				"Not a file or folder", file);
		goto err;
	}
	if(S_ISREG(inode.st_mode)) {
			if(_add_file_qr(file) != 0) {
				LOG(URGENT, "Add to QR Failed");
			}
	} else {
	      	int pipe_fd[2];
	      	char ** symArray = malloc(sizeof(char)*255);
		if (pipe(pipe_fd) < 0) {
			write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
					"Scanner could not pipe");
			free(symArray);
			return;
		}

		if ( (pid = fork() ) < 0) {
			write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
					"Scanner could not fork");
			close (pipe_fd[0]);
			close (pipe_fd[1]);
			free(symArray);
			goto err;
		}

		if (pid == 0) {
			close (pipe_fd[0]);
			if (dup2 (pipe_fd[1], 1) == -1) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
						"Failed to duplicate file descriptor");
				_exit(EXIT_FAILURE);
			}
			char *prog1_argv[7];
			prog1_argv[0] = "find";
			prog1_argv[1] = file;
			prog1_argv[2] = "-type";
			prog1_argv[3] = "f";
			prog1_argv[4] = "!";
			prog1_argv[5] = "-empty";
			prog1_argv[6] = "-print0";
			prog1_argv[7] = NULL;
			if (execvp(prog1_argv[0], prog1_argv) == -1) {
				LOG(WARNING, "Failed to get non-empty files from directory");
				close (pipe_fd[1]);
				exit(EXIT_FAILURE);
			}
			close (pipe_fd[1]);
			exit(EXIT_SUCCESS);
		} else {
			int status;
			waitpid(pid, &status, 0);
			char buf;
		      	char *link = malloc(sizeof(char)*255);
		      	if (link == NULL) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Unable to allocate memory");
				free(symArray);
				return;
		      	}
			close(pipe_fd[1]);
			while (read(pipe_fd[0], &buf, 1) > 0) {
				link[i] = buf;
				i++;
				if(buf == '\0') {
					num++;
					symArray = realloc(symArray, num * sizeof(*symArray));
					if (symArray == NULL) {
						write_to_log(WARNING, "%s - %d - %s", 
							__func__, __LINE__, 
							"Unable to allocate memory");
						free(link);
						close(pipe_fd[0]);
						goto err;
					}
					symArray[num-1] = link;
					link = calloc(255, sizeof(char));
					i = 0;
				}
			}
			free(link);
			close(pipe_fd[0]);
		}
		for(i = 0; i < num; i++) {
			char *commands[3] = {"klearnel", "-add-to-qr", symArray[i]};
			if(qr_query(commands, QR_ADD) != 0) {
				LOG(FATAL, "Add to QR Failed");
			}
			free(symArray[i]);
		}
		if ( (pid = fork() ) < 0) {
			write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
					"Scanner could not fork");
			goto err;
		}
		if (pid == 0) {
			char *prog1_argv[3];
			prog1_argv[0] = "rm";
			prog1_argv[1] = "-rf";
			prog1_argv[2] = file;
			prog1_argv[3] = NULL;
			if (execvp(prog1_argv[0], prog1_argv) == -1) {
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		} else {
			pid_t ws = waitpid(pid, &childExitStatus, WNOHANG);
			if (ws == -1) {
	        		LOG(FATAL, "Folder failed to be deleted");
		        } else {
		        	LOG(INFO, "Folder successfully deleted");
		        }
		}
	}

err:
	return;
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Permanently delete files and folders
  \param        file 	The file or folder to delete
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _permDelete(const char *file) 
{
	struct stat new_s;
	if (stat(file, &new_s) != 0) {
		goto step;
	} 
	if(S_ISDIR(new_s.st_mode)) {
		if (rmdir(file) != 0) {
			write_to_log(WARNING, "%s - %d - %s - %s", 
  				__func__, __LINE__, 
  				"Cannot delete directory", 
  				file);
		}
		return;
	}
	step:
	if (unlink(file) != 0) {
		write_to_log(WARNING, "%s - %d - %s - %s", 
  				__func__, __LINE__, 
  				"Cannot delete file", 
  				file);
	}
	return;
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Compares two files to determine original
  \param        file 	The file currently being treated
  \param 	prev 	The previous file
  \param 	path 	The base path
  \return       char* 	The original file	

  The current file is crosschecked for exclusion, then the md5 sums
  are compared to determine if one of the files are duplicates. To determine
  the original, the file with the oldest st_ctime inode value is kept and
  the duplicate sent to quarantine.
 */
/*--------------------------------------------------------------------------*/
char *_returnOrig(char *file, char *prev, char *path)
{
	int i;
	for(i = 0; i < exclude_num; i++) {
  		if(strstr(file, exclude[i]))
  			return file;
  	}
	
	char *token = malloc(sizeof(char)*255);
	char *token_prv = malloc(sizeof(char)*255);
	if ((token == NULL) || (token_prv == NULL)){
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
			"Unable to allocate memory");
		return file;
      	}
	char *full_file;
	char *full_prev;
	struct stat file_stat, prev_stat;
	if(strncmp(file, prev, MD5) == 0) {
		if (!strcmp(&path[strlen(path)-1], "/")) {
			for(i = 36; i < strlen(file); i++) {
	 			token[i-36] = file[i];
	 		}
		} else {
			LOG_DEBUG;
			for(i = 35; i < strlen(file); i++) {
	 			token[i-35] = file[i];
	 		}
		}
	 	
	 	if (strlen(token) > 0) write_to_log(DEBUG, "token = %s", token);
	 	else write_to_log(DEBUG, "Token is empty");
	 	full_file = malloc(strlen(path)+strlen(token)+1);
	 	if (full_file == NULL){
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			goto out;
	      	}
	 	if (snprintf(full_file, strlen(path)+strlen(token)+1, "%s%s", path, token) <= 0) {
	 		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to print to new variable");
			goto err;
	 	}
	 	if (strlen(full_file) > 0) write_to_log(DEBUG, "Full file = %s", full_file);
	 	else write_to_log(DEBUG, "Full file is empty");
	 	if (!strcmp(&path[strlen(path)-1], "/")) {
			for(i = 36; i < strlen(prev); i++) {
	 			token_prv[i-36] = prev[i];
	 		}
		} else {
			for(i = 35; i < strlen(prev); i++) {
	 			token_prv[i-35] = prev[i];
	 		}
		}
	 	full_prev = malloc(strlen(path)+strlen(token_prv)+1);
	 	if (full_prev == NULL){
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			goto err;
	      	}
	 	if (snprintf(full_prev, strlen(path)+strlen(token_prv)+1, "%s%s", path, token_prv) < 0) {
	 		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to print to new variable");
			goto err2;
	 	}
	 	if(stat(full_file, &file_stat) != 0) {
	 		write_to_log(WARNING, "%s - %d - %s : %s", __func__, __LINE__, 
				"Unable to stat file", full_prev);
			goto err2;
	 	}
	 	if(stat(full_prev, &prev_stat) != 0) {
	 		write_to_log(WARNING, "%s - %d - %s : %s", __func__, __LINE__, 
				"Unable to stat file", full_prev);
			goto err2;
	 	}
	 	if ((int)file_stat.st_ctime < (int)prev_stat.st_ctime) {
	 		_deleteFiles(full_prev);
			goto out_f;
	 	} else {
	 		_deleteFiles(full_file);
			goto out_p;
	 	}
	}
out:
	free(token);
	free(token_prv);
	return file;
err:
	free(token);
	free(token_prv);
	free(full_file);
	return file;
err2:
	free(token);
	free(token_prv);
	free(full_file);
	free(full_prev);
	return file;
out_f:
	free(token);
	free(token_prv);
	free(full_file);
	free(full_prev);
	return file;
out_p:
	free(token);
	free(token_prv);
	free(full_file);
	free(full_prev);
	return prev;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Deletes broken or duplicate symlink
  \param        symlink 	The absolute path of the symlink
  \return       void	

  Crosschecks symlink parameter with global variable protect to
  verify that we aren't crushing temporary symlinks required
  at boot or by system. If crosscheck is OK, symlink is unlinked
 */
/*--------------------------------------------------------------------------*/
  void _deleteSym(const char* symlink)
  {
  	int i;
  	struct stat new_s;
  	for(i = 0; i < protect_num; i++) {
  		if(strncmp(symlink, protect[i], strlen(protect[i])) == 0)
  			return;
  	}
  	if (stat(symlink, &new_s) != 0)  {
  		if (unlink(symlink) != 0) {
  			write_to_log(WARNING, "%s - %d - %s - %s", 
  				__func__, __LINE__, 
  				"Unable to unlink broken symlink", 
  				symlink);
  			return;
  		} else {
  			write_to_log(INFO, "Symlink Deleted : %s",symlink);
  		}
  	}
  }

/*-------------------------------------------------------------------------*/
/**
  \brief        Checks for broken symlinks
  \param        data 	The element to verify
  \return       void	

  Forks, Exec's the find command, outputs result to parent by replacing 
  STDOUT with write end of pipe. Parent then calls _deleteSym for each
  file read in pipecalloc
 */
/*--------------------------------------------------------------------------*/
void _checkSymlinks(TWatchElement data) 
{
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}

	if (!S_ISDIR(inode.st_mode)) {
		return;
	}
	int pid;
      	int pipe_fd[2];

	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not pipe");
		return;
	}

	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not fork");
		goto err;
	}

	if (pid == 0) {
	      	char *prog1_argv[7];

		prog1_argv[0] = "find";
	      	prog1_argv[1] = data.path;
	      	prog1_argv[2] = "-type";
	      	prog1_argv[3] = "l";
	      	prog1_argv[4] = "-xtype";
	      	prog1_argv[5] = "l";
	      	prog1_argv[6] = "-print0";
	      	prog1_argv[7] = NULL;
		close (pipe_fd[0]);
		if (dup2 (pipe_fd[1], 1) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to duplicate file descriptor");
			close (pipe_fd[1]);
			exit(EXIT_FAILURE);
		}
		if (execvp(prog1_argv[0], prog1_argv) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to execute find broken symlinks");
			close (pipe_fd[1]);
			exit(EXIT_FAILURE);
		}
	} else {
		int status;
		int i = 0;
		char buf;
	      	char *link = malloc(sizeof(char)*255);
	      	if (link == NULL) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			return;
	      	}
		close(pipe_fd[1]);
		while (read(pipe_fd[0], &buf, 1) > 0) {
			link[i] = buf;
			i++;
			if(buf == '\0') {
				_deleteSym(link);
				free(link);
				link = malloc(sizeof(char)*255);
				if (link == NULL) {
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, 
						"Unable to allocate memory");
					close(pipe_fd[0]);
					return;
			      	}
				i = 0;
			}
		}
		waitpid(pid, &status, 0);
		close(pipe_fd[0]);
		free(link);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Checks for duplicate symlinks
  \param        data 	The element to verify
  \return       void	

  Forks, Exec's the find command, outputs result to parent by replacing 
  STDOUT with write end of pipe. Dynamic string array stores the final
  destination of each symlink for the current directory being searched
  and if matches one already encountered, it is deleted via the 
  _deleteSym function. Iterates over symlinks until real path

 */
/*--------------------------------------------------------------------------*/
void _dupSymlinks(TWatchElement data)
{
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}

	if (!S_ISDIR(inode.st_mode)) {
		return;
	}
      	int pid;
      	int pipe_fd[2];

	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s", 
				__func__, __LINE__, 
				"Unable to create pipes");
		return;
	}

	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s", 
				__func__, __LINE__, 
				"Unable to fork processes");
		goto err;
	}
	if (pid == 0) {
		char *prog1_argv[5];

		prog1_argv[0] = "find";
	      	prog1_argv[1] = data.path;
	      	prog1_argv[2] = "-type";
	      	prog1_argv[3] = "l";
	      	prog1_argv[4] = "-print0";
	      	prog1_argv[5] = NULL;
		close (pipe_fd[0]);
		if (dup2 (pipe_fd[1], 1) == -1) {
			close (pipe_fd[1]);
			exit(EXIT_FAILURE);
		}
		if (execvp(prog1_argv[0], prog1_argv) == -1) {
			close (pipe_fd[1]);
			exit(EXIT_FAILURE);
		}
	} else {
		int status;
		int i = 0, j = 0;
		int num = 0;
	      	char buf;
	      	char ** symArray;
	      	char *file;
		char *base_path = malloc(sizeof(char)*255);
	      	char *link = malloc(sizeof(char)*255);
	      	char *dir  = malloc(sizeof(char)*255);
	      	if (base_path == NULL || link == NULL || dir == NULL) {
	      		write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, 
						"Unable to allocate memory");
			goto err;
	      	}
		close(pipe_fd[1]);
		while (read(pipe_fd[0], &buf, 1) > 0) {
			link[i] = buf;
			i++;
			if(buf == '\0') {
				if ((file = realpath(link, NULL)) == NULL) {
					goto out;
				}
				memcpy(dir, link, strlen(link));
				dirname(dir);
				if(strcmp(base_path, dir) == 0) {
					num++;
					symArray = realloc(symArray, num * sizeof(*symArray));
					if (symArray == NULL) {
						write_to_log(WARNING, "%s - %d - %s", 
							__func__, __LINE__, 
							"Unable to allocate memory");
						goto err2;
					}
				} else {
					base_path = malloc(strlen(dir));
					if (base_path == NULL) {
						write_to_log(WARNING, "%s - %d - %s", 
							__func__, __LINE__, 
							"Unable to allocate memory");
						free(link);
						free(dir);
						goto err;
					}
					memcpy(base_path, dir, strlen(dir));
					for ( j = 0; j < num; j++ ) {
						free(symArray[j]);
					}
					num = 1;
					symArray = malloc(num * sizeof(*symArray)); // PROTECT
					if (symArray == NULL) {
						write_to_log(WARNING, "%s - %d - %s", 
							__func__, __LINE__, 
							"Unable to allocate memory");
						goto err2;
					}
				}
				symArray[num-1] = malloc(sizeof(char)*255); // PROTECT
				if (symArray[num-1] == NULL) {
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, 
						"Unable to allocate memory");
					goto err3;
				}
				for(j = 0; j < num-1; j++) {
					if (strcmp(symArray[j], file) == 0) {
						if (unlink(link) != 0) {
				  			write_to_log(WARNING, "%s - %d - %s - %s", 
				  				__func__, __LINE__, 
				  				"Unable to destroy duplicate symlink", 
				  				link);
				  		} else {
				  			write_to_log(INFO, "Symlink Deleted : %s",link);
				  		}
						num--; 
						goto out; 
					}
				}
				symArray[num-1] = file;
				out:
				i = 0;
			}

		}
		waitpid(pid, &status, 0);
	err3:
		for ( j = 0; j < num-1; j++ ) {
			free(symArray[j]);
		}
	err2:
		free(base_path);
		free(link);
		free(dir);
		close(pipe_fd[0]);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Checks files and folders of x size to backup
  		or delete
  \param        data 	The element to verify
  \param 	action 	The action to take
  \return       void	

  Forks, Exec's the find command, outputs result to parent by replacing 
  STDOUT with write end of pipe. Parent then calls _deleteFiles
  or _backupFiles depending on action parameter
 */
/*--------------------------------------------------------------------------*/
void _checkFiles(TWatchElement data, int action)
{
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}

	if (S_ISREG(inode.st_mode)) {
		if ( action == SCAN_BACKUP ) {
			_backupFiles(data.path);
		} else {
			_deleteFiles(data.path);
			remove_watch_elem(data);
		}
		return;
	} else if (!S_ISDIR(inode.st_mode)) {
		return;
	}
	int pid;
      	int pipe_fd[2];

	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not pipe");
		return;
	}

	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not fork");
		goto err;
	}

	if (pid == 0) {
	      	char *prog1_argv[7];
	      	double size;
	      	if (action == SCAN_BACKUP) {
	      		size = data.back_limit_size;
	      	} else if (action == SCAN_DEL_F_SIZE) {
	      		size = data.del_limit_size;
	      	}
		prog1_argv[0] = "find";
	      	prog1_argv[1] = data.path;
	      	prog1_argv[2] = "-type";
	      	prog1_argv[3] = "f";
	      	prog1_argv[4] = "-size";
	      	prog1_argv[5] = malloc(strlen("+") + sizeof(double) + strlen("k") + 1);
	      	if (snprintf(prog1_argv[5], strlen("+") + sizeof(int) + strlen("k") + 1, 
	      			"%s%d%s", "+", (int)(size), "M") < 0){
	      		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to print file size limit");
			goto childDeath;
	  	}
	      	prog1_argv[6] = "-print0";
	      	prog1_argv[7] = NULL;
		close (pipe_fd[0]);
		if (dup2 (pipe_fd[1], 1) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to duplicate file descriptor");
			goto childDeath;
		}
		if (execvp(prog1_argv[0], prog1_argv) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to execute find broken symlinks");
			goto childDeath;
		}
	childDeath:
		close (pipe_fd[1]);
		free(prog1_argv[5]);
		exit(EXIT_FAILURE);
	} else {
		int status;
		int i = 0;
		char buf;
	      	char *file = malloc(sizeof(char)*255);
	      	if (file == NULL) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			return;
	      	}
		close(pipe_fd[1]);
		while (read(pipe_fd[0], &buf, 1) > 0) {
			file[i] = buf;
			i++;
			if(buf == '\0') {
				if(action == SCAN_DEL_F_SIZE) {
					_deleteFiles(file);
				} else if (action == SCAN_BACKUP) {
					_backupFiles(file);
				}
				free(file);
				file = malloc(sizeof(char)*255);
				if (file == NULL) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
						"Unable to allocate memory");
					goto err;
				}
				i = 0;
			}
		}
		waitpid(pid, &status, 0);
		close(pipe_fd[0]);
		free(file);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Deletes duplicate files in a folder
  \param        data 	The element to verify
  \param 	action	The action to take
  \return       void	

  Deletes duplicate files, keeping only the most recently modified
  version.
 */
/*--------------------------------------------------------------------------*/
void _handleDuplicates(TWatchElement data, int action) 
{
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}
	if (!S_ISDIR(inode.st_mode)) {
		return;
	}
	int pid;
      	int pipe_fd[2];
	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not pipe");
		return;
	}
	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not fork");
		goto err;
	}
	if (pid == 0) {
		if (chdir(data.path) != 0) {
			_exit(EXIT_FAILURE);
		}
		close (pipe_fd[0]);
		if (dup2 (pipe_fd[1], 1) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Failed to duplicate file descriptor");
			goto childDeath;
		}
		if (system("find -not -empty -type f -printf \"%s\n\" | sort -rn | uniq -d | "
			"xargs -I{} -n1 find -type f -size {}c -print0 | xargs -0 md5sum | "
			"sort | uniq -w32 --all-repeated=separate") == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Failed to execute find duplicates");
			goto childDeath;
		}
	childDeath:
		close (pipe_fd[1]);
		_exit(EXIT_FAILURE);
	} else {
		int status;
		int i = 0;
		char buf;
	      	char *file = malloc(sizeof(char)*255);
	      	char *prev = malloc(sizeof(char)*255);

	      	if ((file == NULL) || (prev == NULL)) {
	      		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			return;
	      	}
	      	strcpy(prev, " ");
		close(pipe_fd[1]);
		char tmp_buf;
		while (read(pipe_fd[0], &buf, 1) > 0) {
			if(buf != '\n') 
				file[i] = buf;
			i++;

			if(buf == '\n') {
				if (tmp_buf != '\n') {
					char prev_tmp[255];
					strcpy(prev_tmp, prev);
					prev = calloc(255, sizeof(char));
					strcpy(prev, _returnOrig(file, prev_tmp, data.path));
				}
				file = calloc(255, sizeof(char));
				if (file == NULL) {
					write_to_log(WARNING, "%s - %d - %s", 
						__func__, __LINE__, 
						"Unable to allocate memory");
					goto err;
				}
				i = 0;
			}
			tmp_buf = buf;
		}
		waitpid(pid, &status, 0);
		close(pipe_fd[0]);
		free(file);
		free(prev);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Checks and repairs incoherent permissions
  \param        data 	The element to verify
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _checkPermissions(TWatchElement data) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Cleans folder at a specified time
  \param        data 	The element to verify
  \return       void	

  Applicable only to temp folders. Forks, then child enters a fork
  loop Exec'ing the find command on basis of table of types, 
  outputs result to parent by replacing STDOUT with write end of 
  pipe. Files are then deleted followed by folders.
 */
/*--------------------------------------------------------------------------*/
void _cleanFolder(TWatchElement data) 
{
	if (!data.isTemp) {
		return;
	}
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}

	if (!S_ISDIR(inode.st_mode)) {
		return;
	}
	int pid;
      	int pipe_fd[2];
      	int i;
	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not pipe");
		return;
	}

	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not fork");
		goto err;
	}

	if (pid == 0) {
	      	char *prog1_argv[5];
	      	char *types[] = {"b", "c", "p", "f", "l", "s", "d"};
	      	int returnStatus, childpid;
		prog1_argv[0] = "find";
	      	prog1_argv[1] = data.path;
	      	prog1_argv[2] = "-type";
	      	prog1_argv[4] = "-print0";
	      	prog1_argv[5] = NULL;
	      	close (pipe_fd[0]);
	      	if (dup2(pipe_fd[1], 1) == -1) {
	      		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Failed to duplicate file descriptor");
			goto childDeath;
		}
		for(i = 0; i < 7; i++) {
			prog1_argv[3] = types[i];
			childpid = fork();
			if(childpid == 0) {
				if (execvp(prog1_argv[0], prog1_argv) == -1) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
					"Failed to execute find broken symlinks");
					goto childDeath;
				}
			} else {
    				waitpid(childpid, &returnStatus, 0);
			}
	      	}
	childDeath:
		close (pipe_fd[1]);
		free(prog1_argv[5]);
		exit(EXIT_FAILURE);
	} else {
		int status;
		i = 0;
		char buf;
	      	char *file = malloc(sizeof(char)*255);
	      	if (file == NULL) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			return;
	      	}
		close(pipe_fd[1]);
		while (read(pipe_fd[0], &buf, 1) > 0) {
			file[i] = buf;
			i++;
			if(buf == '\0') {
				if (strcmp(file, data.path) != 0) {
					_permDelete(file);
				}
				free(file);
				file = malloc(sizeof(char)*255);
				if (file == NULL) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
						"Unable to allocate memory");
					goto err;
				}
				i = 0;
			}
		}
		waitpid(pid, &status, 0);
		close(pipe_fd[0]);
		free(file);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Deletes or backs up files and folders older than X
  \param        data 	The element to verify
  \param 	action	The action to take
  \return       void	

  Forks, Exec's the find command, outputs result to parent by replacing 
  STDOUT with write end of pipe. Parent then calls _deleteFiles
  or _backupFiles depending on action parameter
 */
/*--------------------------------------------------------------------------*/
void _oldFiles(TWatchElement data, int action) 
{
	struct stat inode;
	if (stat(data.path, &inode) != 0) {
		write_to_log(FATAL, "Unable to stat file : %s", data.path);
		return;
	}

	if (S_ISREG(inode.st_mode)) {
		if ( action == SCAN_BACKUP_OLD ) {
			_backupFiles(data.path);
		} else {
			_deleteFiles(data.path);
			remove_watch_elem(data);
		}
		return;
	} else if (!S_ISDIR(inode.st_mode)) {
		return;
	}
	int pid;
      	int pipe_fd[2];

	if (pipe(pipe_fd) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not pipe");
		return;
	}

	if ( (pid = fork() ) < 0) {
		write_to_log(WARNING, "%s - %d - %s",__func__, __LINE__, 
				"Scanner could not fork");
		goto err;
	}

	if (pid == 0) {
	      	char *prog1_argv[8];
	      	uint age = data.max_age;
		prog1_argv[0] = "find";
	      	prog1_argv[1] = data.path;
	      	prog1_argv[2] = "-daystart";
	      	prog1_argv[3] = "-type";
	      	prog1_argv[4] = "f";
	      	prog1_argv[5] = "-atime";
	      	prog1_argv[6] = malloc(strlen("+") + sizeof(uint) + 1);
	      	if (snprintf(prog1_argv[6], strlen("+") + sizeof(uint) + 1, 
	      			"%s%d", "+", (uint)(age)) < 0){
	      		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to print file age limit");
			goto childDeath;
	  	}
	      	prog1_argv[7] = "-print0";
	      	prog1_argv[8] = NULL;
		close (pipe_fd[0]);
		if (dup2 (pipe_fd[1], 1) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to duplicate file descriptor");
			goto childDeath;
		}
		if (execvp(prog1_argv[0], prog1_argv) == -1) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Failed to execute find broken symlinks");
			goto childDeath;
		}
	childDeath:
		close (pipe_fd[1]);
		free(prog1_argv[6]);
		exit(EXIT_FAILURE);
	} else {
		int status;
		waitpid(pid, &status, 0);
		int i = 0;
		char buf;
	      	char *file = malloc(sizeof(char)*255);
	      	if (file == NULL) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
				"Unable to allocate memory");
			return;
	      	}
		close(pipe_fd[1]);
		while (read(pipe_fd[0], &buf, 1) > 0) {
			file[i] = buf;
			i++;
			if(buf == '\0') {
				if(action == SCAN_DEL_F_OLD) {
					_deleteFiles(file);
				} else if (action == SCAN_BACKUP_OLD) {
					_backupFiles(file);
				}
				free(file);
				file = malloc(sizeof(char)*255);
				if (file == NULL) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, 
						"Unable to allocate memory");
					goto err;
				}
				i = 0;
			}
		}
		waitpid(pid, &status, 0);
		close(pipe_fd[0]);
		free(file);
		return;
	}
	err:
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return;
}


int perform_event() 
{
	int i;
	if (watch_list == NULL) {
		goto out;
	}
	
	SCAN_LIST_FOREACH(watch_list, first, next, cur) {
		for(i = 0; i < OPTIONS; i++) {
			switch(i) {
				case SCAN_BR_S :
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_checkSymlinks(cur->element);
						LOG_DEBUG;
					} 
					break;
				case SCAN_DUP_S : 
					if (cur->element.options[i] == '1') { 
						LOG_DEBUG;
						_dupSymlinks(cur->element);
						LOG_DEBUG;
					}
					break;
				case SCAN_BACKUP : 
				case SCAN_DEL_F_SIZE :
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_checkFiles(cur->element, i);
						LOG_DEBUG;
					}
					break;
				case SCAN_DUP_F : 					
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_handleDuplicates(cur->element, i);
						LOG_DEBUG;
					}
					break;
				case SCAN_INTEGRITY : 
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_checkPermissions(cur->element); 
						LOG_DEBUG;
					}
					break;
				case SCAN_CL_TEMP : 
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_cleanFolder(cur->element);
						LOG_DEBUG;
					}
					break;
				case SCAN_DEL_F_OLD : 
				case SCAN_BACKUP_OLD :
					if (cur->element.options[i] == '1') {
						LOG_DEBUG;
						_oldFiles(cur->element, i);
						LOG_DEBUG; 
					}
					break;
				default: break;
			}
		}
	}
out:
	write_to_log(INFO, "%s completed successfully", __func__);
	return 0;
}

int init_scanner()
{
	int o_mask = umask(0);
	if (access(SCAN_DB, F_OK) == -1) {
		if (creat(SCAN_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s:%d: %s", __func__, __LINE__, "Unable to create the scanner database");
			return -1;
		}
	}
	if (access(SCAN_TMP, F_OK) == -1) {
		if (mkdir(SCAN_TMP, S_IRWXU | S_IRWXG | S_IRWXO)) {
			write_to_log(FATAL, "%s:%d: %s", __func__, __LINE__, "Unable to create the SCAN_TMP folder");
			return -1;
		}
	}
	umask(o_mask);
	return 0;
}

int add_watch_elem(TWatchElement elem) 
{
	int i;
	for(i = 0; i < protect_num; i++) {
  		if(strncmp(elem.path, protect[i], strlen(protect[i])) == 0) {
  			LOG(WARNING, "Path is protected, not adding");
  			return -1;
		}
  	}
  	if (strlen(elem.path) <= 1) {
  		LOG(WARNING, "Path is protected, not adding");
  		return -1;
  	}
	TWatchElementNode* node = malloc(sizeof(struct watchElementNode));
	if (!node) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	node->element = elem;
	node->next = NULL;
	if (!watch_list) {
		watch_list = malloc(sizeof(struct watchElementList));
		if (!watch_list) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		watch_list->first = node;
		watch_list->last = node;
		watch_list->count = 1;
		return 0;
	}
	
	node->prev = watch_list->last;
	watch_list->last->next = node;
	watch_list->last = node;
	watch_list->count++;
	return 0;
}

/*-------------------------------------------------------------------------*/
/**
  \brief    Modify the given element in the watch_list
  \param elem The element in the watch_list to modify
  \return   Return 0 on success and -1 on error	

 */
/*--------------------------------------------------------------------------*/
int _mod_watch_elem(TWatchElement elem)
{
	TWatchElementNode* item = watch_list->first;
	if (!item) return 0;
	int i;
	while (item) {
		if (strcmp(item->element.path, elem.path) == 0) {
			for (i = 0; i < strlen(item->element.options); i++) {
				item->element.options[i] = elem.options[i];
			}
			item->element.isTemp = elem.isTemp;
			LOG_DEBUG;
			return 0;
		}
		item = item->next;
	}
	write_to_log(FATAL, "%s:%d: Element \"%s\" to modify not found", 
		__func__, __LINE__, elem.path);
	return -1;
}

int remove_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* item = watch_list->first;
	if (!item) return 0;
	while (item) {
		if (strcmp(item->element.path, elem.path) == 0) {
			if (strcmp(watch_list->first->element.path, item->element.path) == 0) {
				watch_list->first = item->next;
			}
			if (strcmp(watch_list->last->element.path, item->element.path) == 0) {
				watch_list->last = item->prev;
			}
			if (item->next) item->next->prev = item->prev;
			if (item->prev)	item->prev->next = item->next;
			free(item);
			watch_list->count--;
			return 0;
		}
		item = item->next;
	}
	write_to_log(FATAL, "%s:%d: Element \"%s\" to remove not found", 
		__func__, __LINE__, elem.path);
	return -1;
}

int load_watch_list()
{
	int fd = 0;
	TWatchElement tmp;
	if ((fd = open(SCAN_DB, O_RDONLY)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	while(read(fd, &tmp, sizeof(struct watchElement)) != 0) {
		if (add_watch_elem(tmp) < 0) {
			write_to_log(URGENT, "%s:%d: Unable to add \"%s\" to the watch list", 
				__func__, __LINE__, tmp.path);
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}

void clear_watch_list()
{
	if (watch_list->first != NULL) {
		SCAN_LIST_FOREACH(watch_list, first, next, cur) {
			if (cur->prev) {
			    	free(cur->prev);
			}
		}
	}
	free(watch_list);
	watch_list = NULL;
}

/*-------------------------------------------------------------------------*/
/**
  \brief	Get the watch element containing the path given in parameter
  \param 	path the path of the element to obtain
  \return	Return the matched TWatchElement
  
 */
/*--------------------------------------------------------------------------*/
TWatchElement get_watch_elem(const char* path) 
{
	TWatchElement tmp;
	strcpy(tmp.path, EMPTY_PATH);
	if (!watch_list) {
		if (load_watch_list() < 0) {
			LOG(WARNING, "Unable to load the watch list");
			return tmp;			
		}
	}
	int i;
	TWatchElementNode* item = watch_list->first;
	for (i = 0; i < watch_list->count; i++) {
		if (!strcmp(item->element.path, path)) {
			return item->element;
		}
		item = item->next;
	}
	return tmp;
}

int save_watch_list(int custom)
{
	int fd;
	if (watch_list == NULL) return 0;
	if (custom >= 0) {
		fd = custom;
	} else if ((fd = open(SCAN_DB, O_WRONLY | O_TRUNC)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	SCAN_LIST_FOREACH(watch_list, first, next, cur) {
		if (write(fd, &cur->element, sizeof(struct watchElement)) < 0) {
			write_to_log(FATAL, "%s:%s: Unable to write \"%s\" in SCAN_DB", 
				__func__, __LINE__, cur->element.path);
			return -1;
		}
	}
	clear_watch_list();
	if (custom < 0) {
		close(fd);
	}
	return 0;
}

void print_scan(TWatchElementList **list)
{
	clock_t begin, end;
	double spent;
	begin = clock();
	printf("Scanner elements:\n");
	if (*list != NULL) {
		SCAN_LIST_FOREACH((*list), first, next, cur) {
			printf("\nElement \"%s\":\n", cur->element.path);
			printf("\t- Options: %s\n", cur->element.options);
		}
	} else {
		printf("\n\tNothing in Scanner watch list\n");
	}
	end = clock();
	spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("\nQuery executed in: %.2lf seconds\n", spent);
}

void load_tmp_scan(TWatchElementList **list, int fd)
{
	TWatchElement tmp;
	while (read(fd, &tmp, sizeof(struct watchElement)) > 0) {
		if (_add_tmp_watch_elem(tmp, list) != 0){
			perror("Out of memory!");
			exit(EXIT_FAILURE);
		}
	}
}

void scanner_worker()
{
	int len, s_srv, s_cl;
	int c_len = 20;
	int task = 0;
	struct sockaddr_un server;
	int oldmask = umask(0);
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
	umask(oldmask);
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

		if (watch_list != NULL) {
			clear_watch_list();
		}
		if (load_watch_list() < 0) {
			LOG(WARNING, "Unable to load the watch list");
			return;
		}

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
				perform_task(task, buf, s_cl);
				free(buf);
				close(s_cl);
			}
		} else {
			perform_event();
		}
	} while (task != KL_EXIT);
	close(s_srv);
	unlink(server.sun_path);
	exit(EXIT_SUCCESS);
}

int perform_task(const int task, const char *buf, const int s_cl) 
{
	switch (task) {
		case SCAN_ADD: 
			if (buf == NULL) {
				LOG(URGENT, "Buffer received is empty");
				return -1;
			}
			int fd = open(buf, O_RDONLY);
			if (fd <= 0) {
				write_to_log(URGENT,"%s:%d: Unable to open %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			TWatchElement new;
			if (read(fd, &new, sizeof(struct watchElement)) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to read data from %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			close(fd);
			if (add_watch_elem(new) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to add %s to watch_list", 
					__func__, __LINE__, new.path);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;
			}
			unlink(buf);
			save_watch_list(-1);
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		case SCAN_MOD:
			if (buf == NULL) {
				LOG(URGENT, "Buffer received is empty");
				return -1;
			}
			int fd_mod = open(buf, O_RDONLY);
			if (fd_mod <= 0) {
				write_to_log(URGENT,"%s:%d: Unable to open %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			TWatchElement mod;
			if (read(fd_mod, &mod, sizeof(struct watchElement)) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to read data from %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			close(fd_mod);
			if (_mod_watch_elem(mod) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to modify options of %s in watch_list", 
					__func__, __LINE__, mod.path);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;
			}
			unlink(buf);
			save_watch_list(-1);
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		case SCAN_RM:
			if (buf == NULL) {
				LOG(URGENT, "Buffer received is empty");
				return -1;				
			}
			TWatchElement tmp = get_watch_elem(buf);
			if (!strcmp(tmp.path, EMPTY_PATH)) {
				LOG(NOTIFY, "Unable to find the element to remove");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			if (remove_watch_elem(tmp) < 0) {
				write_to_log(NOTIFY, "%s:%d: Unable to remove %s", 
					__func__, __LINE__, tmp.path);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			save_watch_list(-1);
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		case SCAN_LIST: ;
			if (watch_list == NULL) {
				if (write(s_cl, VOID_LIST, strlen(VOID_LIST)) < 0) {
					LOG(WARNING, "Unable to send VOID_LIST");
					return -1;
				}
				return 0;
			}
			time_t timestamp = time(NULL);
			int tmp_stock;
			char *path_to_list = malloc(sizeof(char)*(sizeof(timestamp)+20));
			char *file = malloc(sizeof(char)*(sizeof(timestamp)+30));
			if (!path_to_list || !file) {
				write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				return 0;
			}
			sprintf(path_to_list, "%s", SCAN_TMP);
			int oldmask = umask(0);
			if (access(path_to_list, F_OK) == -1) {
				if (mkdir(path_to_list, ALL_RWX)) {
					write_to_log(WARNING, "%s - %d - %s:%s", __func__, __LINE__, "Unable to create the tmp_stock folder", path_to_list);
					if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
						write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
					}
					free(path_to_list);
					return 0;
				}
			}
			sprintf(file, "%s/%d", path_to_list, (int)timestamp);
			tmp_stock = open(file, O_WRONLY | O_TRUNC | O_CREAT, ALL_RWX);
			if (tmp_stock < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open tmp_stock");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				free(path_to_list);
				free(file);
				return 0;
			}
			save_watch_list(tmp_stock);
			close(tmp_stock);
			umask(oldmask);
			write(s_cl, file, PATH_MAX);
			free(path_to_list);
			free(file);
			break;
		case KL_EXIT:
			write_to_log(INFO, "Scanner received stop command");
			exit_scanner();
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		case RELOAD_CONF:
			reload_config();
			if (SOCK_ANS(s_cl, SOCK_ACK) < 0) {
				LOG(URGENT, "Unable to send ACK");
				return -1;
			}
			break;
		default:
			LOG(NOTIFY, "Unknown task. Scan execution aborted");
	}

	return 0;
}

int exit_scanner()
{
	if (save_watch_list(-1) < 0) {
		LOG(FATAL, "Unable to save the watch_list");
		return -1;
	}
	return 0;
}
