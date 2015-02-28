#ifndef _KLEARNEL_QUARANTINE_H
#define _KLEARNEL_QUARANTINE_H
/*
 * This file contains all structure and includes
 * needed by the quarantine routines
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#define QR_STOCK 	WORK_DIR "/qr_stock"
#define QR_DB 		BASE_DIR "/qr.db"
#define QR_SOCK 	TMP_DIR "/kl-qr-sck"

#define QR_ADD 		1
#define QR_RM 		2
#define QR_REST		3
#define QR_LIST 	4
#define QR_INFO		5
#define QR_EXIT		0

#define IPC_QR 		42

/* Structure of file into quarantine */
struct qr_file {
	char 		f_name[256];		/* Filename of the fil in qr */
	char		o_path[PATH_MAX]; 	/* Old path to restore */
	struct stat	o_ino; 			/* Inode info before move into quarantine */
	time_t 		d_begin; 		/* Date at which file has been moved to quarantine */
	time_t 		d_expire; 		/* Date at which file will be dropped out (optional) */
};

typedef struct qr_file QrData;

typedef struct qr_node *QrPosition;
typedef struct qr_node *QrSearchTree;

/* List of file into the quarantine */
struct qr_node {
	QrData		data;
	QrSearchTree	left;
	QrSearchTree	right;
};



/*----- PROTOYPE ------ */

void init_qr();

void load_tmp_qr(QrSearchTree *list, int fd);

void load_qr(QrSearchTree *list);

QrSearchTree clear_qr_list(QrSearchTree list);

QrPosition search_in_qr(QrSearchTree list, char *filename);

int save_qr_list(QrSearchTree *list, int other);

int add_file_to_qr(QrSearchTree *list, char *filepath);

int rm_file_from_qr(QrSearchTree *list, char *filename);

int restore_file(QrSearchTree *list, char *filename);

void qr_worker();

void print_qr(QrSearchTree list);

#endif /* _KLEARNEL_QUARANTINE_H */
