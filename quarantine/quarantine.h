#ifndef _KLEARNEL_QUARANTINE_H
#define _KLEARNEL_QUARANTINE_H
/*
 * This file contains all structure and includes
 * needed by the quarantine routines
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>

#define QR_STOCK "stock/"
#define QR_DB "quarantine.db"

/* Structure of file into quarantine */
struct qr_file {
	int 		id;
	char 		f_name[PATH_MAX + 1];
	struct 		stat old_inode; /* inode info before move into quarantine */
	time_t 		d_begin; /* date at which file has been moved to quarantine */
	time_t 		d_expire; /* date at which file will be dropped out (optional) */
};

/* List of file into the quarantine */
struct qr_list {
	struct qr_list* 	next;
	struct qr_file*		f_info;
	struct qr_list* 	prev;
};

#endif /* _KLEARNEL_QUARANTINE_H */