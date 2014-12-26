/*
 * Files that require user actions are stored in quarantine
 * It can be used as backup zone
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include "quarantine.h"

/* Extract all informations of a new file into the qr_list */
void extract_file_info(const struct dirent _file, struct qr_list* _stock){
	int fd;
	struct qr_file* tmp;
	struct stat tmpStat;

	char* path = malloc(strlen(STOCK_QR));
	strcpy(path, QR_STOCK);
	strcat(path, _file.d_name);

	stat(path, &tmpStat);

	tmp = malloc(sizeof(struct qr_file));

	tmp->old_inode = tmpStat;
	tmp->f_name = path;
	tmp->d_begin = time(NULL);
	tmp->d_expire = 0;

	if(_stock->f_info != NULL) 
		tmp->id = _stock->f_info.id + 1;
	else tmp->id = 0;

	if((fd = open(QR_DB, 0_APPEND | O_CREAT, USER_RW)) < 0){
		perror("%s : Unable to open STOCK_DB file", __func__);
		return;
	}

	if(write(fd, tmp, sizeof(struct qr_file)) <= 0){
		perror("%s : Unable to write new file in QR_DB file", __func__);
		return;
	}

	close(fd);

	struct qr_list* new_f = malloc(sizeof(struct qr_list));
	new_f->f_info = tmp;
	if(_stock->f_info != NULL) new_f->prev = _stock;
	else new_f->prev = NULL;
	new_f->next = NULL;
	_stock->next = new_f;
}

/* Move file to STOCK_QR */
void add_file_to_qr(const char* _add, const struct qr_file _info){
	if(rename(_add, _info.f_name)){
		perror("%s : Unable to move %s to the quarantine", __func__, _add);
		return;
	}
}

/* Load _stock with content of QR_DB */
void load_qr(struct qr_list* _stock){
	int fd;
	struct qr_file* tmp;
	
	if(fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0){
		perror("%s : Unable to open QR_DB file", __func__);
		return;
	}

	while(read(fd, tmp, sizeof(struct qr_file)) != 0){
		struct qr_list* new_f = malloc(sizeof(struct qr_list));
		new_f.f_info = tmp;
		if(_stock->f_info != NULL) new_f.prev = _stock;
		else new_f.prev = NULL;
		new_f.next = NULL;
		_stock->next = new_f;
	}

	close(fd);
}