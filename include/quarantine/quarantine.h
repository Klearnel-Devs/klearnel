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

/* Structure of file into quarantine */
struct qr_file {
	char 		f_name[256];		/* Filename of the fil in qr */
	char		o_path[PATH_MAX]; 	/* Old path to restore */
	struct stat	o_ino; 			/* Inode info before move into quarantine */
	time_t 		d_begin; 		/* Date at which file has been moved to quarantine */
	time_t 		d_expire; 		/* Date at which file will be dropped out (optional) */
};

typedef struct qr_file QrData;

struct QRListNode;

typedef struct QRListNode {
    struct QRListNode *next;
    struct QRListNode *prev;
    QRData data;
} QRListNode;

typedef struct QRList {
    int count;
    QRListNode *first;
    QRListNode *last;
} QRList;

#define List_count(A) ((A)->count)
#define List_first(A) ((A)->first != NULL ? (A)->first->value : NULL)
#define List_last(A) ((A)->last != NULL ? (A)->last->value : NULL)

#define LIST_FOREACH(L, S, M, V) QRListNode *_node = NULL;\
    QRListNode *V = NULL;\
    for(V = _node = L->S; _node != NULL; V = _node = _node->M)

/*----- PROTOYPE ------ */

void init_qr();

void load_tmp_qr(QRList *list, int fd);

void load_qr(QRList *list);

void clear_qr_list(QRList *list);

void search_in_qr(QRList *list, char *filename);

int save_qr_list(QRList *list, int custom);

int add_file_to_qr(QRList *list, char *filepath);

int rm_file_from_qr(QRList *list, char *filename);

int restore_file(QRList *list, char *filename);

void qr_worker();

void print_qr(QRList *list);

#endif /* _KLEARNEL_QUARANTINE_H */
