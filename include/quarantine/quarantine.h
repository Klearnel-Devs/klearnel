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
#define QR_TMP      TMP_DIR "/quarantine"
#define QR_SOCK 	TMP_DIR "/kl-qr-sck"
#define RES_DEF     "/tmp/klearnel"

#define QR_ADD 		    1
#define QR_RM 		    2
#define QR_REST		    3
#define QR_LIST 	    4
#define QR_INFO		    5
#define QR_RM_ALL       6
#define QR_REST_ALL     7
#define QR_LIST_RECALL  8

#define EXP_DEF    2592000

/* Structure of file into quarantine */
struct qr_file {
	char 		f_name[256];		/* Filename of the fil in qr */
	char		o_path[PATH_MAX]; 	/* Old path to restore */
	struct stat	o_ino; 			/* Inode info before move into quarantine */
	time_t 		d_begin; 		/* Date at which file has been moved to quarantine */
	time_t 		d_expire; 		/* Date at which file will be dropped out (optional) */
};

typedef struct qr_file QrData;

struct QrListNode;

typedef struct QrListNode {
    struct QrListNode *next;
    struct QrListNode *prev;
    QrData data;
} QrListNode;

typedef struct QrList {
    int count;
    QrListNode *first;
    QrListNode *last;
} QrList;

#define LIST_FOREACH(L, S, M, V) QrListNode *_node = NULL;\
    QrListNode *V = NULL;\
    for(V = _node = (*L)->S; _node != NULL; V = _node = _node->M)

/*----- PROTOYPE ------ */

void init_qr();

void load_tmp_qr(QrList **list, int fd);

void load_qr(QrList **list);

void clear_qr_list(QrList **list);

QrListNode* search_in_qr(QrList *list, char *filename);

int save_qr_list(QrList **list, int custom);

int add_file_to_qr(QrList **list, char *filepath);

int rm_file_from_qr(QrList **list, char *filename);

int restore_file(QrList **list, char *filename);

void qr_worker();

void print_qr(QrList **list);

#endif /* _KLEARNEL_QUARANTINE_H */
