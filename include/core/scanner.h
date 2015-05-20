/*-------------------------------------------------------------------------*/
/**
   \file	scanner.h
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Header for Scanner module
*/
/*--------------------------------------------------------------------------*/

#ifndef _KLEARNEL_SCANNER_H
#define _KLEARNEL_SCANNER_H

/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/

#define SCAN_DB 	BASE_DIR "/scan.db"
#define SCAN_SOCK	TMP_DIR	"/kl-scan-sck"
#define SCAN_TMP 	TMP_DIR "/scan"

#define OPTIONS   11

#define SCAN_ADD	10
#define SCAN_RM		11
#define SCAN_LIST	12

#define SCAN_BR_S	0
#define SCAN_DUP_S	1
#define SCAN_BACKUP	2
#define SCAN_DEL_F_SIZE	3
#define SCAN_DUP_F	4
#define SCAN_FUSE	5
#define SCAN_INTEGRITY	6
#define SCAN_CL_TEMP	7
#define SCAN_DEL_F_OLD	8
#define SCAN_BACKUP_OLD 9
#define SCAN_OPT_END 	10 /* Need to be set to last character in the option string for \0 */

#define EMPTY_PATH	"empty"

/*---------------------------------------------------------------------------
                                New types
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  \brief	Structure for folder/file to be supervised by Scanner

  	 Option list of actions to perform in binary format:
	  Char 0 : Delete broken symlinks 		_only applicable to folders_
	  Char 1 : Delete duplicate symlinks 		_only applicable to folders_
	  Char 2 : Backup files larger than X size 	_applicable to folders and files 
	 						if backup location is configured_
	  Char 3 : Delete files larger than X size 	_applicable to folders and files_
	  Char 4 : Delete duplicate files 		_only applicable to folders_
	  Char 5 : Fuse duplicate files 		_only applicable to folders_
	  Char 6 : Fix permissions integrity 		_only applicable to folders_
	  Char 7 : Clean the folder at specified time 	_only applicable to temp folders_
	  Char 8 : Delete files older than X time 	_applicable to folders and files_
	  Char 9 : Backup files older than X time	_applicable to folders and files 
	 						if backup location is configured_

 */
/*-------------------------------------------------------------------------*/
typedef struct watchElement {
	char path[PATH_MAX];	 
	char options[OPTIONS]; 
	double back_limit_size;	//!<  Used by option 2
	double del_limit_size;	//!<  Used by option 3
	bool isTemp; 		//!<  Used by option 7
	int max_age; 		//!<  Used by option 8
} TWatchElement;

/*-------------------------------------------------------------------------*/
/**
  \brief	Structure for list node of Scanner
 */
/*-------------------------------------------------------------------------*/
typedef struct watchElementNode {
	struct watchElement element;
	struct watchElementNode* next; //!< Pointer to next watchlistElement
	struct watchElementNode* prev; //!< Pointer to previous watchlistElement
} TWatchElementNode;

/*-------------------------------------------------------------------------*/
/**
  \brief	Structure for linked list of Scanner
 */
/*-------------------------------------------------------------------------*/
typedef struct watchElementList {
	int count;
	struct watchElementNode* first; //!< Pointer to first watchlistElement
	struct watchElementNode* last;  //!<  Pointer to last watchlistElement
} TWatchElementList;

/*---------------------------------------------------------------------------
                                Macros
 ---------------------------------------------------------------------------*/

#define SCAN_LIST_FOREACH(L, S, M, V) TWatchElementNode *_node = NULL;\
    TWatchElementNode *V = NULL;\
    for(V = _node = L->S; _node != NULL; V = _node = _node->M)

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  \brief	  Initializes Scanner Module
  \return	  Return 0 on success and -1 on error

  Create the SCAN_DB if it doesn't exist
 */
/*--------------------------------------------------------------------------*/
int init_scanner();
/*-------------------------------------------------------------------------*/
/**
  \brief 	  Get the command through the socket
  \return	  void

  
 */
/*--------------------------------------------------------------------------*/
/*  */
void scanner_worker();
/*-------------------------------------------------------------------------*/
/**
  \brief    Load the scan list from SCAN_DB
  \return   Return 0 on success and -1 on error	

 */
/*--------------------------------------------------------------------------*/

int load_watch_list();
/*-------------------------------------------------------------------------*/
/**
  \brief 	Add a new folder/file to supervise to the scan list
  \param	list: Can be used to specify another list than default
  \return	Return 0 on success and -1 on error

  List parameter must be set to NULL if not used
 */
/*--------------------------------------------------------------------------*/
int add_watch_elem(TWatchElement new);
/*-------------------------------------------------------------------------*/
/**
  \brief 	Remove a folder/file from the scan list
  \param	New 	Element to remove
  \return	Return 0 on success and -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int remove_watch_elem(TWatchElement elem);
/*-------------------------------------------------------------------------*/
/**
  \brief	Load temporary Scanner file
  \param	list 	The list to load
  \param 	fd 	File descriptor
  \return	void

  Used by user to list files in scanner
 */
/*--------------------------------------------------------------------------*/
void load_tmp_scan(TWatchElementList **list, int fd);
/*-------------------------------------------------------------------------*/
/**
  \brief Send the scan list to user by socket 
  \return Return 0 on success and -1 on error	

 */
/*--------------------------------------------------------------------------*/
int get_watch_list();
/*-------------------------------------------------------------------------*/
/**
  \brief 	Clear the watch list
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
void clear_watch_list();
/*-------------------------------------------------------------------------*/
/**
  \brief	Write the scan list to SCAN_DB
  \param	custom  can be used to save the list into another file than default db
  \return	Return 0 on success and -1 on error

  Custom must be set to -1 if not used
 */
/*--------------------------------------------------------------------------*/
int save_watch_list(int custom);
/*-------------------------------------------------------------------------*/
/**
  \brief 	Print all elements in the scanner watch list
  \param	list 	The list of elements to print
  \return	void

  
 */
/*--------------------------------------------------------------------------*/
/* */
void print_scan(TWatchElementList **list);
/*-------------------------------------------------------------------------*/
/**
  \brief	Execute the task ordered by the user
  \param	
  \return	Return 0 on success and -1 on error

  
 */
/*--------------------------------------------------------------------------*/
/* 
 * 
 */
int perform_task(const int task, const char* buf, const int s_cl);
/*-------------------------------------------------------------------------*/
/**
  \brief	Execute the classic scan task
  \param 	task 	The task to execute
  \param 	buf 	Argument of task to execute
  \param 	s_cl 	The socket file descriptor
  \return	Return 0 on success and -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int perform_event();
/*-------------------------------------------------------------------------*/
/**
  \brief	Exit the scanner process
  \return	Return 0 on success and -1 on error

 */
/*--------------------------------------------------------------------------*/
int exit_scanner();

#endif /* _KLEARNEL_SCANNER_H */