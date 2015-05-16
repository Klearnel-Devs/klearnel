/*-------------------------------------------------------------------------*/
/**
   \file	logging.h
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Logging header

   This file contains all structure and includes needed by 
   the logging routines
*/
/*--------------------------------------------------------------------------*/
#ifndef _KLEARNEL_LOGGING_H
#define _KLEARNEL_LOGGING_H
/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include <dirent.h>
#include <stdarg.h>
/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/
#define FATAL 		5
#define URGENT 		4
#define WARNING		3
#define NOTIFY		2
#define INFO		1
#define DEBUG		0
#define OLD		2592000
#define IPC_LOG 48

#define LOG_DEBUG \
 	write_to_log(DEBUG," Function: %s:%d", __func__,__LINE__)

#define LOG(level, msg) \
	write_to_log(level, "%s:%d: %s", __func__, __LINE__, msg)
/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  \brief 	Initializes the logging semaphore	
  \return	void

 */
/*--------------------------------------------------------------------------*/
void init_logging();
/*-------------------------------------------------------------------------*/
/**
  \brief	Function for writing messages to program log files
  \param 	level 	Log Notification Level
  \param 	format 	Variable argument format
  \param 	... 	Variable arguments
  \return	Returns -1 in case of error, 0 if succeeded (to be removed)

  Entries are preceeded by time -> severity level -> message
  Semaphore used to guarantee that only one process may write at a time

 */
/*--------------------------------------------------------------------------*/
int write_to_log(int level, char const *format, ...);

#endif /* _KLEARNEL_LOGGING_H */
