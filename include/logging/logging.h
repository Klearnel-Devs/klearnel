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
/**
  \brief Define fatal level

  The program has been unexpectly closed
 */                                
#define FATAL 		5
/**
  \brief Define urgent level 

  The function has been aborted
 */
#define URGENT 		4
/**
  \brief Define warning level

  The function is not aborted but the user is informed
  that an error occured
 */
#define WARNING		3
/**
  \brief Define notify level

  Used to notify a specific action has been performed
 */
#define NOTIFY		2
/**
  \brief Define info level

  Used for normal message
 */
#define INFO		1
/**
  \brief Define debug level

  Only used by developers during the development
 */
#define DEBUG		0
/**
  \brief Define the default maximum age of a log file
 */
#define OLD		2592000

/**
  \brief Define the IPC number of log sema
 */
#define IPC_LOG 48

/**
  \brief Write a debug message in log file

  The message structure is:
    Function: function_name:line_number
 */
#define LOG_DEBUG \
 	write_to_log(DEBUG," Function: %s:%d", __func__,__LINE__)
/**
  \brief Write a message in log file
  \param level The level of the message
  \param msg The message to write
 */
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
  \brief        Deletes old log files
  \return       void

  Function that iterates through the log files contained in the Klearnel
  log directory defined in global.h. Each log files last accessed time is
  compared to the current time, and if older than desired, deleted
 */
/*--------------------------------------------------------------------------*/
void delete_logs();
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
