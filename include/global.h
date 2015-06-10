/*-------------------------------------------------------------------------*/
/**
   \file	global.h
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Global header file

   Contains all prototype, includes & constants used by Klearnel 
*/
/*--------------------------------------------------------------------------*/
#ifndef _KLEARNEL_GLOBAL_H
#define _KLEARNEL_GLOBAL_H

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
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
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#endif
#include <sys/wait.h>
/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/

#define ALL_R 		S_IRUSR | S_IRGRP | S_IROTH
#define USER_RW		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define ALL_RWX		S_IRWXU | S_IRWXG | S_IRWXO

#define BASE_DIR 	"/etc/klearnel"
#define WORK_DIR	"/usr/local/klearnel"
#define LOG_DIR		"/var/log/klearnel/"
#define TMP_DIR		"/tmp/.klearnel"
#define PID_FILE	BASE_DIR "/klearnel.pid"
#define BASH_AUTO "/etc/bash_completion.d"
#define KLEARNEL_AUTO BASH_AUTO "/klearnel"
#define AUTO_TMP  TMP_DIR "/ac"
#define IPC_RAND 	"/dev/null"
#define IPC_PERMS 	0666

#define SOCK_NET_PORT	42225

#define SOCK_ACK 	"1"
#define SOCK_NACK 	"2"
#define SOCK_DENIED 	"3"
#define SOCK_UNK	"4"
#define SOCK_ABORTED 	"8"
#define SOCK_RETRY 	"9"

#define KL_EXIT 	-1

#define SOCK_TO 	15 /* Define the timeout applied to sockets */
#define SEL_TO 		600 /* Default = 600 */ /* Define the timeout waiting on sockets */

#define SOCK_ANS(socket, signal) \
 	write(socket, signal, strlen(signal))

#define List_count(A) 	((A)->count)
#define List_first(A) 	((A)->first != NULL ? (A)->first->value : NULL)
#define List_last(A) 	((A)->last != NULL ? (A)->last->value : NULL)

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief	Get data from socket
  \param 	sock 	Socket to retrieve data from
  \param 	action 	Action to take
  \param 	buf 	Buffer to store data from socket
  \param  	c_len 	Length of data
  \return	Return number of char read if >= 0, else -1

  
 */
/*--------------------------------------------------------------------------*/
int get_data(const int sock, int *action, char **buf, int c_len);

/*-------------------------------------------------------------------------*/
/**
  \brief	Signal critical area linked to sema is now used
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void sem_down(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Signal critical area linked to sema is now free
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void sem_up(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Reset the sema to 1
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void sem_reset(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Check if the critical area linked to sema sem_id is accessible
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	Returns ???

  
 */
/*--------------------------------------------------------------------------*/
int is_crit_area_free(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Wait for critical area freeing
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void wait_crit_area(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Return the value of sem_channel in the sema sem_id
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \return	Returns ???

  
 */
/*--------------------------------------------------------------------------*/
int sem_val(int sem_id, int sem_channel);
/*-------------------------------------------------------------------------*/
/**
  \brief	Set sem_channel in the sema sem_id at val
  \param 	sem_id 		Semaphore ID
  \param 	sem_channel 	Semaphore Channel
  \param 	val 		Value to set sema to
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void sem_put(int sem_id, int sem_channel, int val);
/*-------------------------------------------------------------------------*/
/**
  \brief	Prints notice if function not implemented
  \param 	func 	Function name
  \return	void

  Set this when new functions suspected to be called 
  but not yet implemented to crash the module without
  any error message
 */
/*--------------------------------------------------------------------------*/
void not_yet_implemented(const char* func);
#define NOT_YET_IMP not_yet_implemented(__func__);

#endif /* _KLEARNEL_GLOBAL_H */
