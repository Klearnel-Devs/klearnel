/*
 * This file is the main one for the klearnel module
 * Please read README.md for more information 
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <core/ui.h>

/* Initialize all components required by the module */
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
	init_qr();
}

/* Daemonize the module */
void _daemonize()
{
	NOT_YET_IMP;
}

/* Save the main process ID
 * to allow restart and stop actions
 * and to have a program status
 * Return 0 on success, -1 on error
 */
int _save_main_pid(pid_t pid)
{
	int fd;
	char *pid_s;
	if ((pid_s = malloc(PID_MAX_S)) == NULL) {
		perror("KL: Unable to allocate memory");
		return -1;
	}
	snprintf(pid_s, PID_MAX_S, "%d", pid);
	if (access(PID_FILE, F_OK) == -1) {
		if (creat(PID_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) {
			perror("KL: Unable to create the pid file");
			return -1;
		}
	}
	if ((fd = open(PID_FILE, O_WRONLY, S_IWUSR)) < 0) {
		perror("KL: Unable to open pid file");
		return -1;
	}
	if (write(fd, pid_s, strlen(pid_s)) < 0) {
		perror("KL: Unable to write process id in the pid file");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/* Main function of the module 
 * It will initialize all components
 * and create the other processes required
 * Return 0 on success, -1 on error
 */
int main(int argc, char **argv)
{
	int pid;
	
	if (argc > 1) {
		execute_commands(argc, argv);
		return EXIT_SUCCESS;
	}
	_init_env();
	if (_save_main_pid(getpid())) {
		perror("KL: Unable to save the module pid");
		return EXIT_FAILURE;
	}

	pid = fork();
	if (pid == 0) {
		qr_worker();
	} else if (pid > 0) {
		/* Parent will call the other processes */
		/* DO NOTHING AT THIS TIME */
		/* NEEDS TO BE APPROVED */
	} else {
		perror("KL: Unable to fork first processes");
		return EXIT_FAILURE;
	}

	/* will be deamonized later */
	return EXIT_SUCCESS;
}
