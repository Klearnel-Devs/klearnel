/*
 * Files that require user actions are stored in quarantine
 * It can be used as backup zone
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include "quarantine.h"


/* Load _stock with content of QR_DB */
struct qr_list* load_qr(){
	int fd;
	struct qr_file* tmp;
	struct qr_list* stock_qr;

	if(fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0){
		perror("%s : Unable to open QR_DB file", __func__);
		return;
	}

	while(read(fd, tmp, sizeof(struct qr_file)) != 0){
		struct qr_list* new_f = malloc(sizeof(struct qr_list));
		new_f.f_info = tmp;
		if(stock_qr->f_info != NULL) new_f.prev = stock_qr;
		else new_f.prev = NULL;
		new_f.next = NULL;
		stock_qr->next = new_f;
	}
	close(fd);
	return stock_qr;
}

struct qr_file search_in_qr(const char* _file){

}

void cancel_add(const char* cur_p, const char* old_path){

}

/* Extract all informations of a new file into the qr_list */
void extract_file_info(const char* _file, const char* new_path){
	int fd;
	struct qr_file* tmp;
	struct qr_list* stock_qr = load_qr();
	struct stat tmpStat;

	char* path = malloc(strlen(QR_STOCK));
	strcpy(path, QR_STOCK);
	strcat(path, _file);

	stat(new_path, &tmpStat);

	tmp = malloc(sizeof(struct qr_file));

	tmp->old_inode = tmpStat;
	tmp->old_path  = _file;
	tmp->f_name    = _file->d_name;
	tmp->d_begin   = time(NULL);
	tmp->d_expire  = 0;

	if((fd = open(QR_DB, 0_APPEND | O_CREAT, USER_RW)) < 0){
		perror("%s : Unable to open STOCK_DB file", __func__);
		cancel_add(path, _file);
		free(stock_qr);
		return;
	}

	if(write(fd, tmp, sizeof(struct qr_file)) <= 0){
		perror("%s : Unable to write new file in QR_DB file", __func__);
		cancel_add();
	}

	close(fd);
	free(stock_qr);


}

/* Move file to STOCK_QR */
void add_file_to_qr(const char* _file){
	struct dirent to_add = dirent()
	char* path = malloc(strlen(QR_STOCK));
	strcpy(path, QR_STOCK);
	strcat(path, _file);
	if(rename(_file, path)){
		perror("%s : Unable to move %s to the quarantine", __func__, _add);
		return;
	}
	extract_file_info(_file, path);
}

/* Delete definitively a file from the quarantine */
void rm_file_from_qr(const char* _file){

}

/* Restore file to its anterior state */
void restore_file(const char* _file){

}