#ifndef _KLEARNEL_GLOBAL_H
#define _KLEARNEL_GLOBAL_H
/*
 * Global header file
 * Contains all prototype, includes & constants used by the whole module 
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include <libgen.h>
#include <ctype.h>

/* ------ CONSTANTS ------ */

#define ALL_R 		S_IRUSR | S_IRGRP | S_IROTH
#define USER_RW		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define BASE_DIR 	"/etc/klearnel"
#define WORK_DIR	"/usr/local/klearnel"
#define LOG_DIR		"/var/log/klearnel/"
#define TMP_DIR		"/tmp/.klearnel"
#define PID_FILE	BASE_DIR "/klearnel.pid"

#define IPC_RAND 	"/dev/null"
#define IPC_PERMS 	0666

#define SOCK_ACK 	"1"
#define SOCK_NACK 	"2"
#define SOCK_DENIED 	"3"
#define SOCK_UNK	"4"
#define SOCK_ABORTED 	"8"
#define SOCK_RETRY 	"9"

#define KL_EXIT 	-1

#define SOCK_TO 	15 /* Define the timeout applied to sockets */
#define SEL_TO 		600 /* Define the timeout waiting on sockets */

#define SOCK_ANS(socket, signal) \
 	write(socket, signal, strlen(signal))

/* ------ PROTOTYPES ----- */

/* Get data from socket "sock" and put it in buffer "buf"
 * Return number of char read if >= 0, else -1
 */
int get_data(const int sock, int *action, char **buf, int c_len);


/* Signal critical area linked to sema is now used */
void sem_down(int sem_id, int sem_channel);

/* Signal critical area linked to sema is now free */
void sem_up(int sem_id, int sem_channel);

/* Reset the sema to 1 */
void sem_reset(int sem_id, int sem_channel);

/* Check if the critical area linked to sema sem_id is accessible */
int is_crit_area_free(int sem_id, int sem_channel);

/* Wait for critical area freeing */
void wait_crit_area(int sem_id, int sem_channel);

/* Return the value of sem_channel in the sema sem_id */
int sem_val(int sem_id, int sem_channel);

/* Set sem_channel in the sema sem_id at val */
void sem_put(int sem_id, int sem_channel, int val);

/* Set this when new functions suspected to be called 
 * but not yet implemented to crash the module without
 * any error message
 */ 
void not_yet_implemented(const char* func);
#define NOT_YET_IMP not_yet_implemented(__func__);

#endif /* _KLEARNEL_GLOBAL_H */
