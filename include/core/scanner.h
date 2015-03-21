#ifndef _KLEARNEL_SCANNER_H
#define _KLEARNEL_SCANNER_H
/*
 * Header file for Scanner module
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 */

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

/* Structure for chained list of Scanner */
typedef struct watchElementList {
	struct watchElement element;
	struct watchElementList* next;
	struct watchElementList* prev;
} TWatchElementList;

#endif /* _KLEARNEL_SCANNER_H */