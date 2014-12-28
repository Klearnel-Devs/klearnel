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

	elem = new_qr_node();
	elem->data  = new;

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

void cancel_add(const char* cur_p, const char* old_path){

}

/* Extract all informations of a new file into the qr_list */
void extract_file_info(const char* file, const char* new_path){

}

/* Move file to STOCK_QR */
void add_file_to_qr(const char* file){

}

/* Delete definitively a file from the quarantine */
void rm_file_from_qr(const char* file){

}

/* Restore file to its anterior state and place */
void restore_file(const char* file){

}