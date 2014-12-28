/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include "quarantine.h"

/* Init a node of the quarantine with all attributes at NULL */
struct qr_node* new_qr_node(){
	struct qr_node* tmp = malloc(sizeof(struct qr_node));
	if(tmp == NULL){
		perror("%s: Couldn't allocate memory", __func__);
		return NULL;
	}
	tmp->data = NULL;
	tmp->left = NULL;
	tmp->right = NULL;
	return tmp;
}

/* Add a file to the quarantine list */
void add_to_qr_list(struct qr_node** list, struct qr_file* new){
	struct qr_node *tmpNode;
	struct qr_node *tmpList = *list;
	struct qr_node *elem;

	if(elem = new_qr_node()) == NULL){
		perror("%s: Unable to create new qr node", __func__);
		return;
	}

	elem->data = new;

	if(tmpList){
		do{
			tmpNode = tmpList;
			if(new->o_ino.st_ino > tmpList->data->o_ino.st_ino)
			{
		            	tmpList = tmpList->right;
		            	if(!tmpList) tmpNode->right = elem;
		        }
		        else 
		        {
		            	tmpList = tmpList->left;
		            	if(!tmpList) tmpNode->left = elem;
		        }
	    	} while(tmpList);
	}
	else  *list = elem;
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void clear_qr_list(struct node** list){
	struct node* tmp = *list;
	
	if(!list) return;
	if(tmp->left)  clear_qr_list(&tmp->left);
	if(tmp->right) clear_qr_list(&tmp->right);

	free(tmp);
	*list = NULL;
}

/* Load quarantine with content of QR_DB 
 * Return double pointer to avoid data copy
 */
struct qr_node** load_qr(){
	int fd;
	struct qr_file *tmp;
	struct qr_node *list = NULL;

	if((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0){
		int err = errno;
		if(err == EACCESS) return new_qr_node();
		perror("%s: Unable to open %s", __func__, QR_DB);
		return NULL;
	}

	while(read(fd, tmp, sizeof(qr_file)) != 0)
		add_to_qr_list(&list, tmp);

	close(fd);
	return &list;
}

/* Search for a node in the list on inode number base */
struct qr_node* search_in_qr(struct qr_node *list, const ino_t inum){
	struct qr_node *tmpList = list;
	struct qr_node *tmpNode;
        
	tmpNode = new_qr_node();

	while(list){

		ino_t cur_ino = tmpList->data->o_ino->st_ino;

		if(inum == cur_ino) break;
		else if(inum > cur_ino) tmpList = tmpList->right;
		else tmpList = tmpList->left; 
	}

	tmpNode = tmpList;

	return tmpNode;
}

void cancel_add(const char *cur_p, const char *old_p){
	if(rename(cur_p, old_p)){
		perror("%s: Unable to move file %s to %s", __func__, cur_p, old_p);
		return;
	}
}

/* Get filename in a path */
char* get_filename(const char *path){
	char sep = '/';
	char *name;
	int i, l_occ, lg_tot, lg_sub;

  	lg_tot = strlen(path);
	for(i = 0; i < lg_tot; i++) 
		if(path[i] == sep) 
			l_occ = i;

	if((name = malloc(lg_tot - i) + 1)) == NULL){
		perror("%s: Couldn't allocate memory", __func__);
		return NULL;
	}

	for(i = 0; i < l_occ; i++){
		path++;
		lg_sub = i;
	}

	for(i = 0; i < (lg_tot - lg_sub); i++){
		*(name + i) = *path;
		path++;
	} 

	return name;
}

/* Write the list into the quarantine DB */
void save_qr_list(struct node** list){
	int fd;

	if((fd = open(QR_DB, O_WRONLY, S_IRUSR)) < 0){
		perror("%s: Unable to open the QR_DB", __func__);
		return;
	}

	// TODO
}

/* Move file to STOCK_QR */
void add_file_to_qr(const char* file){
	struct node *list = *load_qr();
	struct qr_file *new_f = malloc(sizeof(struct qr_file));
	struct stat new_s;
	char *new_path = malloc(sizeof(QR_STOCK));
	char *fn = get_filename(file);

	if((new_f == NULL) || (new_path == NULL )){
		perror("%s: Couldn't allocate memory", __func__);
		return;
	}

	strcpy(path, QR_STOCK);
	strcat(path, fn);

	stat(new_path, &new_s);

	strcpy(new_f->old_inode, new_s);
	strcpy(new_f->old_path, file);
	strcpy(new_f->f_name, fn);

	if((new_f->f_name == NULL)
		return;

	new_f->d_begin   = time(NULL);
	/* This will be changed when config implemented 
	 * Choice will be between expire configured and not configured
	 */
	new_f->d_expire  = 0;

	add_to_qr_list(&list, new_f);
	save_qr_list(list);

        if(rename(file, new_path)){
        	perror("%s: Unable to move %s to %s", __func__, file, new_path);
        	return;
        }

        clear_qr_list(&list);
}

/* Delete definitively a file from the quarantine */
void rm_file_from_qr(const char* file){

}

/* Restore file to its anterior state and place */
void restore_file(const char* file){

}