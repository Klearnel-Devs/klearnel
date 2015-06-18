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
/**
 \brief The scanner database file
*/ 
#define SCAN_DB 	BASE_DIR "/scan.db"
/**
 \brief The Scanner Unix Domain Socket file
*/ 
#define SCAN_SOCK	TMP_DIR	"/kl-scan-sck"
/**
 \brief The temporary Scanner folder
*/ 
#define SCAN_TMP 	TMP_DIR "/scan"
/**
 \brief Size of the md5 checksum
*/ 
#define MD5       35
/**
 \brief Number of Scanner options
*/ 
#define OPTIONS   10
/**
 \brief The add to Scanner signal
*/ 
#define SCAN_ADD	10
/**
 \brief The remove from Scanner signal
*/ 
#define SCAN_RM		11
/**
 \brief The get list from Scanner signal
*/ 
#define SCAN_LIST	12
/**
 \brief The modify elem in Scanner signal
*/ 
#define SCAN_MOD  13
/**
 \brief Force the scanner to execute perform_event()
*/
#define SCAN_FORCE 14
/*-------------------------------------------------------------------------*/
/**
 \brief "Delete broken symlinks" option position
*/ 
#define SCAN_BR_S	0
/**
 \brief "Delete duplicate symlinks" option position
*/ 
#define SCAN_DUP_S	1
/**
 \brief "Backup files larger than X size" option position
*/ 
#define SCAN_BACKUP	2
/**
 \brief "Delete files larger than X size" option position
*/ 
#define SCAN_DEL_F_SIZE	3
/**
 \brief "Delete duplicate files" option position
*/ 
#define SCAN_DUP_F	4
/**
 \brief "Fix permissions integrity" option position
*/ 
#define SCAN_INTEGRITY	5
/**
 \brief "Clean the folder at specified time" option position
*/ 
#define SCAN_CL_TEMP	6
/**
 \brief "Delete files older than X time" option position
*/ 
#define SCAN_DEL_F_OLD	7
/**
 \brief "Backup files older than X time" option position
*/ 
#define SCAN_BACKUP_OLD 8
/**
 \brief  Need to be set to last character in the option string for \0 
*/ 
#define SCAN_OPT_END 	9

/**
 \brief Signal to inform that a path is empty
*/ 
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
	  Char 5 : Fix permissions integrity 		_only applicable to folders_
	  Char 6 : Clean the folder at specified time 	_only applicable to temp folders_
	  Char 7 : Delete files older than X time 	_applicable to folders and files_
	  Char 8 : Backup files older than X time	_applicable to folders and files 
	 						if backup location is configured_

 */
/*-------------------------------------------------------------------------*/
typedef struct watchElement {
	char path[PATH_MAX];	//!< The path of the element
	char options[OPTIONS]; //!< The options list in char array form
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
	struct watchElement element; //!< The element in the node
	struct watchElementNode* next; //!< Pointer to next watchlistElement
	struct watchElementNode* prev; //!< Pointer to previous watchlistElement
} TWatchElementNode;

/*-------------------------------------------------------------------------*/
/**
  \brief	Structure for linked list of Scanner
 */
/*-------------------------------------------------------------------------*/
typedef struct watchElementList {
	int count; //!< The number of elements in the list
	struct watchElementNode* first; //!< Pointer to first watchlistElement
	struct watchElementNode* last;  //!<  Pointer to last watchlistElement
} TWatchElementList;

/*---------------------------------------------------------------------------
                                Macros
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief  Macro to traverse the watch_list
 */
/*-------------------------------------------------------------------------*/
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
  \param	new The element to add to the watch_list
  \return	Return 0 on success and -1 on error

 */
/*--------------------------------------------------------------------------*/
int add_watch_elem(TWatchElement new);
/*-------------------------------------------------------------------------*/
/**
  \brief 	Remove a folder/file from the scan list
  \param	elem	The element to remove
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
  \param  task  The task to execute
  \param  buf   Argument of task to execute
  \param  s_cl  The socket file descriptor
  \return	Return 0 on success and -1 on error

  
 */
/*--------------------------------------------------------------------------*/
/* 
 * 
 */
int perform_task(const int task, const char* buf, const int s_cl);
/*-------------------------------------------------------------------------*/
/**
  \brief	Execute the recurrent scan tasks
  \return	Return 0 on success and -1 on error

  This function execute the specific actions for each entry in the wath_list
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