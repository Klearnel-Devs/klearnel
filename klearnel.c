/*-------------------------------------------------------------------------*/
/**
   \file	klearnel.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Klearnel File

  This file is the main one for the Klearnel module
  Please read README.md for more information 
 
  Copyright (C) 2014, 2015 Klearnel-Devs
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
/*--------------------------------------------------------------------------*/
 
#include <global.h>
#include <quarantine/quarantine.h>
#include <core/scanner.h>
#include <core/ui.h>
#include <logging/logging.h>
#include <config/config.h>
#include <net/crypter.h>
#include <net/network.h>

/*-------------------------------------------------------------------------*/
/**
  \brief        Creates Klearnel Bash Autocompletion
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void autocomplete()
{
	FILE *ac;
	if ((ac = fopen(KLEARNEL_AUTO, "w")) == NULL)
		goto err;
	fprintf(ac,
	"_klearnel()\n"
	"{\n"
	"	local cur prev opts\n"
	"	COMPREPLY=()\n"
	"	cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
	"	prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n"
	"	opts=\"-add-to-qr -rm-from-qr -rm-all-from-qr -get-qr-list\n"
	"	-get-qr-info -restore-from-qr -restore-all-from-qr\n"
	"	-add-to-scan -rm-from-scan -get-scan-list -view-rt-log\n" 
	"	-license -start -restart -stop -help -force-scan -flush\""
	"\n"
	"	if [[ ${cur} == -* ]] ; then\n"
	"		COMPREPLY=( $(compgen -W \"${opts}\" -- ${cur}) )\n"
	"		return 0\n"
	"	elif [[ ${prev} == -* ]] ; then\n"
	"		_filedir\n"
	"	fi\n"
	"}\n"
	"complete -F _klearnel klearnel\n");
	fclose(ac);

	if ((ac = fopen(AUTO_TMP, "w")) == NULL)
		goto err;
	fprintf(ac, "#!/bin/bash\n"
		". /etc/bash_completion.d/klearnel");
	fclose(ac);
	
	system("chmod +x /tmp/.klearnel/ac");
	system("/tmp/.klearnel/ac");
	unlink(AUTO_TMP);
	return;
	err:
		printf("INIT: Could not create/source klearnel bash autocompletion file");
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Initialize all components required by the module
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void _init_env()
{
	if (access("/etc/", W_OK) == -1) {
		fprintf(stderr, "Klearnel needs to be launched with root permissions!\n");
		exit(EXIT_FAILURE);
	}

	if (access(BASE_DIR, F_OK) == -1) {
		if (mkdir(BASE_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("KL: Unable to create the base directory");
			exit(EXIT_FAILURE);
		}		
	}

	if (access(WORK_DIR, F_OK) == -1) {
		if (mkdir(WORK_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("KL: Unable to create the configuration directory");
			exit(EXIT_FAILURE);
		}		
	}

	if (access(TMP_DIR, F_OK) == -1) {
		int oldmask = umask(0);
		if (mkdir(TMP_DIR, 0777)) {
			perror("KL: Unable to create the temp directory");
			exit(EXIT_FAILURE);
		}	
		umask(oldmask);	
	}

	if (access(BASH_AUTO, F_OK) == 0) {
		if(access(KLEARNEL_AUTO, F_OK) == -1) {
			autocomplete();
		}
	}

	init_logging();
	init_qr();
	init_scanner();
	init_config();
	delete_logs();
	int is_gen = -1;
	while(is_gen < 0) is_gen = generate_token();
	encrypt_root();
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Daemonize the module
  \return       void

  
 */
/*--------------------------------------------------------------------------*/
void _daemonize()
{
	pid_t pid, sid;

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);       

	sid = setsid();
	if (sid < 0) {
		write_to_log(FATAL, "%s: Unable to create new session id for Klearnel", __func__);
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		write_to_log(FATAL, "%s: Unable to change root directory of Klearnel", __func__);
		exit(EXIT_FAILURE);
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);        
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Saves the main process ID for operations
  \param        pid 	The PID of the main Klearnel process
  \return       0 on success, -1 on error

  Saves the main process ID to allow restart and stop actions
  and to have a program status
 */
/*--------------------------------------------------------------------------*/
int _save_main_pid(pid_t pid)
{
	int fd;
	char *pid_s;
	if ((pid_s = malloc(sizeof(char)*25)) == NULL) {
		perror("KL: Unable to allocate memory");
		return -1;
	}
	if (snprintf(pid_s, sizeof(pid_s), "%d", pid) < 0) {
		write_to_log(FATAL, "[KL] Unable to create the pid file path");
		goto error;
	}
	if (access(PID_FILE, F_OK) == -1) {
		if (creat(PID_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			perror("KL: Unable to create the pid file");
			goto error;
		}
	}
	if ((fd = open(PID_FILE, O_WRONLY, S_IWUSR)) < 0) {
		perror("KL: Unable to open pid file");
		goto error;
	}
	if (write(fd, pid_s, strlen(pid_s)) < 0) {
		perror("KL: Unable to write process id in the pid file");
		close(fd);
		goto error;
	}
	close(fd);
	free(pid_s);
	return 0;
error:
	free(pid_s);
	return -1;
}

/*-------------------------------------------------------------------------*/
/**
 \brief		Get the saved process ID
 \return 	The saved process ID, -1 on error, -2 if no pid stored

  Get the process ID stored in the file PID_FILE (if exist) to allow 
  status verification and restart action
 */
/*-------------------------------------------------------------------------*/
int _get_stored_pid()
{
	int fd;
	int pid;
	char *pid_s;
	if ((pid_s = malloc(sizeof(char)*25)) == NULL) {
		perror("KL: Unable to allocate memory");
		return -1;
	}
	if (access(PID_FILE, F_OK) == -1) {
		free(pid_s);
		return -2;
	}

	if ((fd = open(PID_FILE, O_RDONLY, S_IRUSR)) < 0){
		perror("KL: Unable to open pid file");
		goto error;
	}

	if (read(fd, pid_s, strlen(pid_s)) < 0) {
		perror("KL: Unable to open pid file");
		close(fd);
		goto error;
	}

	pid = (int)strtoimax(pid_s, NULL, 10);

	free(pid_s);
	close(fd);
	return pid;
error:
	free(pid_s);
	return -1;
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Check if Klearnel is already running
  \return       0 on success, -1 on error, -2 if already running

*/
/*--------------------------------------------------------------------------*/
int _check_other_instance() 
{
	int old_pid = _get_stored_pid();
	if (old_pid == -1){
		printf("[\e[31mNOK\e[0m]\n");
		perror("\nKL: Error on old PID check up");
		return -1;
	} else if (old_pid != -2){
		if (kill((pid_t)old_pid, 0) == 0) {
			char *com;
			char output[PATH_MAX];

			if ((com = malloc(sizeof(char)*256)) == NULL) {
				perror("KL: Unable to allocate memory");
				return -1;				
			}

			if (snprintf(com, sizeof(com), "/bin/ps -p %d -o comm=", old_pid) <= 0) {
				perror("KL: Unable to create check up command");
				free(com);
				return -1;
			}

			FILE *fp;
			fp = popen(com, "r");

			if (fp == NULL){
				perror("KL: Failed to run pid check up");
				free(com);
				return -1;
			}


			if (fgets(output, sizeof(output)-1, fp) == NULL) {
				perror("KL: Failed to get check up proces ID response");
				pclose(fp);
				free(com);
				return -1;
			}

			if (!strcmp(output, "klearnel -start") || !strcmp(output, "klearnel -restart") || !strcmp(output, "klearnel")) {
				printf("[\e[31mNOK\e[0m]\n");
				pclose(fp);
				free(com);
				return -2;
			}

			pclose(fp);
			free(com);
		}		
	}
	return 0;	
}

/*-------------------------------------------------------------------------*/
/**
  \brief        Main function of the module
  \param        argc 	Number of launch arguments
  \param        argv	Launch arguments
  \return       0 on success

  It will initialize all components and create the other processes required
 */
/*--------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	int pid;
	if (argc <= 1) {
		printf("Klearnel: missing parameter(s)\n"
		       "Enter \"klearnel -help\" for further information\n");
		return EXIT_SUCCESS;
	}
	if (!strcmp(argv[1], "-start")) {
		goto service;
	} 

	if (!strcmp(argv[1], "-restart")) {
		int stop_status = stop_action();
		if (!stop_status) {
			printf("\n");
			goto service;
		}

	}

	execute_commands(argc, argv);
	return EXIT_SUCCESS;

service: ;
	printf("Starting Klearnel services...\t");
	_init_env();
	/* ---------------- CHECK PROGRAM STATUS ------------------- */
	int check_resp = _check_other_instance();
	if (check_resp == -2) {
		goto other_instance;
	} else if (check_resp == -1) {
		return EXIT_FAILURE;
	}
	/* ---------------- KEARNEL MUTEX INIT --------------------- */
	key_t mutex_key = ftok(IPC_RAND, IPC_MUTEX);
	int mutex = semget(mutex_key, 1, IPC_CREAT | IPC_EXCL | IPC_PERMS);
	if (mutex < 0) {
		if (errno == EEXIST) {
			printf("[\e[31mNOK\e[0m]\n");
			goto other_instance;
		} else {
			printf("[\e[31mNOK\e[0m]\n");
			printf("\nUnable to start Klearnel service: mutex couldn't be created\n");
		}
		return EXIT_FAILURE;
	}

	if (_save_main_pid(getpid())) {
		printf("[\e[31mNOK\e[0m]\n");
		LOG(WARNING, "KL: Unable to save the module pid");
		return EXIT_FAILURE;
	}
	
	/* --------------- SERVICES START -------------------------- */
	printf("[\e[32mOK\e[0m]\n");
	_daemonize();
	pid = fork();
	if (pid == 0) {
		qr_worker();
	} else if (pid > 0) {
		int pid_net = fork();
		if (pid_net == 0) {
			networker();
		} else if (pid_net > 0) {
			scanner_worker();
		} else {
			LOG(FATAL, "KL: Unable to fork for Network & Scanner processes");
			free_cfg(1);
			return EXIT_FAILURE;
		}
	} else {
		LOG(FATAL, "KL: Unable to fork for Quarantine & Scanner processes");
		free_cfg(1);
		return EXIT_FAILURE;
	}
	free_cfg(1);

	semctl(mutex, 0, IPC_RMID, NULL);
	return EXIT_SUCCESS;

other_instance:
	printf("\nKlearnel is already running!\n"
		"If you think that Klearnel services are not running, "
		"Please execute: klearnel -flush and try to start the services again\n");
	return EXIT_FAILURE;
}
