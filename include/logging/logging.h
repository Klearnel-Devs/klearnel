#ifndef _KLEARNEL_LOGGING_H
#define _KLEARNEL_LOGGING_H
/*
 * This file contains all structure and includes
 * needed by the logging routines
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <dirent.h>
#include <stdarg.h>
 
/* Defines log severity */
#define FATAL 		5
#define URGENT 		4
#define WARNING		3
#define NOTIFY		2
#define INFO		1
#define DEBUG		0

/* TEMPORARY DEFINITION OF LOG MAX AGE */
#define OLD		2592000

/* Logging Semaphore */
#define IPC_LOG 48

/*----- PROTOYPE ------ */

int write_to_log(int level, char const *format, ...);

#define LOG_DEBUG \
 	write_to_log(DEBUG," Function: %s", __func__)

#define LOG(level, msg) \
	write_to_log(level, "%s:%d: %s", __func__, __LINE__, msg)

void init_logging();

#endif /* _KLEARNEL_LOGGING_H */
