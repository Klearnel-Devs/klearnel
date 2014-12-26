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

/* ------ CONSTANTS ------ */

#define ALL_R 		S_IRUSR | S_IRGRP | S_IROTH
#define USER_RW		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#endif /* _KLEARNEL_GLOBAL_H */