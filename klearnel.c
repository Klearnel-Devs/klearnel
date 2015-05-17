/*-------------------------------------------------------------------------*/
/**
   \file	klearnel.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Klearnel File

  This file is the main one for the klearnel module
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
	"	opts=\"add-to-qr rm-from-qr rm-all-from-qr get-qr-list\n"
	"	get-qr-info restore-from-qr restore-all-from-qr\n"
	"	add-to-scan rm-from-scan view-rt-log license start\n"
	"	stop help\""
	"\n"
	"	if [[ ${cur} == * ]] ; then\n"
	"		COMPREPLY=( $(compgen -W \"${opts}\" -- ${cur}) )\n"
	"		return 0\n"
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
	write_to_log(INFO, "Klearnel bash autocompletion file successfully created");
	return;
	err:
		write_to_log(WARNING, "Could not create/source klearnel bash autocompletion file");
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
	NOT_YET_IMP;
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
	if ((pid_s = malloc(PID_MAX_S)) == NULL) {
		perror("KL: Unable to allocate memory");
		return -1;
	}
	if (snprintf(pid_s, PID_MAX_S, "%d", pid) < 0) {
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
		       "Enter \"klearnel help\" for further information\n");
		return EXIT_SUCCESS;
	}
	if (!strcmp(argv[1], "start")) {
		goto service;
	} 
	execute_commands(argc, argv);
	return EXIT_SUCCESS;

service:
	_init_env();
	if (_save_main_pid(getpid())) {
		perror("KL: Unable to save the module pid");
		return EXIT_FAILURE;
	}
	
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
			perror("KL: Unable to fork for Network & Scanner processes");
			free_cfg();
			return EXIT_FAILURE;
		}
	} else {
		perror("KL: Unable to fork for Quarantine & Scanner processes");
		free_cfg();
		return EXIT_FAILURE;
	}
	free_cfg();
	 

	/* will be deamonized later */
	return EXIT_SUCCESS;
}
