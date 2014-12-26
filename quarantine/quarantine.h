#include <global.h>

#define STOCK_QR "stock/"

/* Structure of file into quarantine */
struct qr_file {
	int id;
	char* f_name;
	struct stat old_inode; /* inode info before move into quarantine */
	time_t d_begin; /* date at which file has been moved to quarantine */
	time_t d_expire; /* date at which file will be dropped out (optional) */
};

/* List of file into the quarantine */
struct qr_list {
	struct qr_list* next;
	struct qr_file f_info;
	struct qr_list* prev;
};

