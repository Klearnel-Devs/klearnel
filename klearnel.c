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

/* Initialize all components required by the module */
void _init_env()
{
	if (access(BASE_DIR, F_OK) == -1) {
		if (mkdir(BASE_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("KL: Unable to create the base directory");
			exit(EXIT_FAILURE);
		}		
	}
	if (access(CONF_DIR, F_OK) == -1) {
		if (mkdir(CONF_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("KL: Unable to create the configuration directory");
			exit(EXIT_FAILURE);
		}		
	}
	init_qr();
}

/* Daemonize the module */
void _daemonize()
{

}

/* Save the main process ID
 * to allow restart and stop actions
 * and to have a program status
 */
int _save_main_pid(pid_t pid)
{
	int fd;
	if (access(PID_FILE, F_OK) == -1) {
		if (creat(PID_FILE, S_IRWXU | S_IRGRP | S_IROTH)) {
			perror("KL: Unable to create the pid file");
			return -1;
		}
	}
	if ((fd = open(PID_FILE, O_WRONLY, S_IWUSR)) < 0) {
		perror("KL: Unable to open pid file");
		return -1;
	}
	if (write(fd, &pid, sizeof(pid)) < 0) {
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
 */
int main(int argc, char** argv)
{
	int pid;
	_init_env();
	
	if (argc > 1) {
		execute_command(argc, argv);
		return EXIT_SUCCESS;
	}
	if (_save_main_pid(getpid())) {
		perror("KL: Unable to save the module pid");
		return EXIT_FAILURE;
	}

	pid = fork();
	if (pid == 0) {
		/* Child process will load command parts */
	} else if (pid > 0) {
		/* parent will call the */
	} else {
		perror("KL: Unable to fork first processes");
		return EXIT_FAILURE;
	}

	/* will be deamonized later */
	return EXIT_SUCCESS;
}
