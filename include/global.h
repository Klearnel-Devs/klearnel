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
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

/* ------ CONSTANTS ------ */

#define ALL_R 		S_IRUSR | S_IRGRP | S_IROTH
#define USER_RW		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define QR_SHM_SIZE 0x4C4B40 /* Fixed at 5 MB */ 

#define IPC_RAND "/dev/null"
#define IPC_PERMS 0666

#define QR_SHM 101
#define QR_SEM 102

/* ------ PROTOTYPES ----- */

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

#endif /* _KLEARNEL_GLOBAL_H */