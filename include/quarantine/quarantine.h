#ifndef _KLEARNEL_QUARANTINE_H
#define _KLEARNEL_QUARANTINE_H
/*
 * This file contains all structure and includes
 * needed by the quarantine routines
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#define QR_STOCK BASE_DIR "/qr_stock"
#define QR_DB BASE_DIR "/qr.db"

/* Structure of file into quarantine */
struct qr_file {
	char 		f_name[PATH_MAX + 1];
	char		o_path[PATH_MAX + 1];
	struct stat	o_ino; /* inode info before move into quarantine */
	time_t 		d_begin; /* date at which file has been moved to quarantine */
	time_t 		d_expire; /* date at which file will be dropped out (optional) */
};

/* List of file into the quarantine */
struct qr_node {
	struct qr_file*		data;
	struct qr_node* 	left;
	struct qr_node* 	right;
};

/*----- PROTOYPE ------ */

void init_qr();

void load_qr(struct qr_node **list);

int add_to_qr_list(struct qr_node **list, struct qr_file *new_f);

struct qr_node *search_in_qr(struct qr_node *list, const char *filename);

int save_qr_list(struct qr_node **list);

void add_file_to_qr(struct qr_node **list, const char *file);

int rm_file_from_qr(struct qr_node **list, const char *file);

int restore_file(struct qr_node **list, const char *file);

#endif /* _KLEARNEL_QUARANTINE_H */
