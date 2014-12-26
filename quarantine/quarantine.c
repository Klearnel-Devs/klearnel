/*
 * Files that require user actions are stored in quarantine
 * It can be used as backup zone
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include "quarantine.h"
// TODO: Changer cette merde avec un fichier contenant les données des fichiers stockés en quarantaine
struct qr_file extract_file_info(struct dirent* _file, struct qr_list* _stock){
	struct qr_file tmp;
	struct stat tmpStat;

	char* path = malloc(strlen(STOCK_QR));
	strcpy(path, STOCK_QR);
	strcat(path, _file->d_name);

	stat(path, &tmpStat);

	tmp.old_inode = tmpStat;
	tmp.f_name = path;
	tmp.d_begin = time(NULL);
	tmp.d_expire = 0;

	if(_stock->prev != NULL) 
		tmp.id = _stock->prev->id + 1;
	else tmp.id = 0;

	return tmp;
}

int add_file_to_qr(struct qr_file* _add, struct qr_list* _stock){
	// need to know the absolute path to file _add
	
}

void load_qr(struct qr_list* _stock){
	DIR* base;
	struct dirent* file;
	if((base = opendir(STOCK_QR)) == NULL){
		perror("Unable to open the quarantine stock directory\n");
		return;
	}
	while((file = readdir(base)) != NULL){
		if(!strcmp(file->d_name, ".") || 
		   !strcmp(file->d_name, ".."))
			continue;

	}
}