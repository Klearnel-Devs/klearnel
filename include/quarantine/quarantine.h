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

#define IPC_QR 42

/* Structure of file into quarantine */
struct qr_file {
	char 		f_name[PATH_MAX + 1];
	char		o_path[PATH_MAX + 1];
	struct stat	o_ino; /* inode info before move into quarantine */
	time_t 		d_begin; /* date at which file has been moved to quarantine */
	time_t 		d_expire; /* date at which file will be dropped out (optional) */
};

typedef struct qr_file QrData;

struct qr_node;
typedef struct qr_node* QrPosition;
typedef struct qr_node* QrSearchTree;

/* List of file into the quarantine */
struct qr_node {
	QrData		data;
	QrSearchTree 	left;
	QrSearchTree	right;
};

/*----- PROTOYPE ------ */

QrSearchTree load_qr();

QrSearchTree add_to_qr_list(QrSearchTree list, QrData new_f);

QrPosition search_in_qr(QrSearchTree list, const char *filename);

int save_qr_list(QrSearchTree list);

QrSearchTree add_file_to_qr(QrSearchTree list, const char *filepath);

QrSearchTree rm_file_from_qr(QrSearchTree list, const char *filename);

QrSearchTree restore_file(QrSearchTree list, const char *filename);

void qr_worker();

#endif /* _KLEARNEL_QUARANTINE_H */
