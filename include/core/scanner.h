#ifndef _KLEARNEL_SCANNER_H
#define _KLEARNEL_SCANNER_H
/*
 * Header file for Scanner module
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 */

#define SCAN_DB 	BASE_DIR "/scan.db"

/* Structure for folder/file to be supervised by Scanner */
typedef struct watchElement {
	char path[PATH_MAX];
	/* Option list of actions to perform in binary format:
	 * Char 0 : Delete broken symlinks _Only applicable to folders_
	 * Char 1 : Delete duplicate symlinks _Only applicable to folders_
	 * Char 2 : Backup files larger than X size _applicable to folders and files if backup location is configured_
	 * Char 3 : Delete files larger than X size _applicable to folders and files_
	 * Char 4 : Delete duplicate files _applicable to folders and files_
	 * Char 5 : Fuse duplicate files _applicable to folders and files_
	 * Char 6 : Fix permissions integrity _Only applicable to folders_
	 * Char 7 : Clean the folder at specified time _Only applicable to temp folders_
	 * Char 8 : Delete files older than X time _applicable to folders and files_
	 */
	char options[10]; 
	int limit_size;		/* Used by options 2 and 3 */
	bool isTemp; 		/* Used by option 7 */
	float max_age; 		/* Used by option 8 */
} TWatchElement;

/* Structure for list node of Scanner */
typedef struct watchElementNode {
	struct watchElement element;
	struct watchElementNode* next;
	struct watchElementNode* prev;
} TWatchElementNode;

/* Structure for linked list of Scanner */
typedef struct watchElementList {
	int count;
	struct watchElementNode* first;
	struct watchElementNode* last;
} TWatchElementList;

/* List of prototypes */

/* Create the SCAN_DB if it doesn't exist
 * Return 0 on success and -1 on error
 */
int init_scanner();
/* Get the command through the socket */
void get_command();
/* Load the scan list from SCAN_DB 
 * Return 0 on success and -1 on error
 */
int load_watch_list();
/* Add a new folder/file to supervise to the scan list
 * Return 0 on success and -1 on error
 */
int add_watch_elem(TWatchElement new);
/* Remove a folder/file from the scan list
 * Return 0 on success and -1 on error
 */
int remove_watch_elem(TWatchElement elem);
/* Send the scan list to user by socket 
 * Return 0 on success and -1 on error
 */
int get_watch_list();
/* Write the scan list to SCAN_DB
 * Return 0 on success and -1 on error
 */
int save_watch_list();
/* Execute the task ordered by the user or by the timer event
 * Return 0 on success and -1 on error
 */
int perform_task(int task);
/* Exit the scanner process
 * Return 0 on success and -1 on error
 */
int exit_scanner();

#endif /* _KLEARNEL_SCANNER_H */