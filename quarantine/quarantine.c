/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>

/* Initialize all requirements for Quarantine */
void init_qr(){
	if(access(QR_STOCK, F_OK) == -1){
		if(mkdir(QR_STOCK, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)){
			perror("QR: Unable to create the stock");
			return;
		}
	}
	if(access(QR_DB, F_OK) == -1){
		if(creat(QR_DB, S_IRWXU | S_IRGRP | S_IROTH)){
			perror("QR: Unable to create the database file");
			return;
		}
	}
}

/* Init a node of the quarantine with all attributes at NULL 
 * Return 0 on success, -1 on error
 */
int new_qr_node(struct qr_node **new_node){
	*new_node = malloc(sizeof(struct qr_node));
	if(*new_node == NULL){
		perror("Couldn't allocate memory");
		return -1;
	}
	(*new_node)->data = NULL;
	(*new_node)->left = NULL;
	(*new_node)->right = NULL;
	return 0;
}

/* Add a file to the quarantine list 
 * Return 0 on success, -1 on error
 */
int add_to_qr_list(struct qr_node **list, struct qr_file *new){
	struct qr_node *tmpNode;
	struct qr_node *tmpList = *list;
	struct qr_node *elem;

	if(new_qr_node(&elem)){
		perror("QR: Unable to create new qr node");
		return -1;
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
	return 0;
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void clear_qr_list(struct qr_node **list){
	struct qr_node *tmp = *list;
	
	if(!list) return;
	if(tmp->left)  clear_qr_list(&tmp->left);
	if(tmp->right) clear_qr_list(&tmp->right);

	free(tmp);
	*list = NULL;
}

/* Load quarantine with content of QR_DB  
 */
void load_qr(struct qr_node **list){
	int fd;
	struct qr_file *tmp;
	clear_qr_list(list);

	if((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0){
		perror("Unable to open QR_DB");
		return;
	}

	if((tmp = malloc(sizeof(struct qr_file))) == NULL){
		perror("Unable to allocate memory");
		close(fd);
		return;
	}
	while(read(fd, tmp, sizeof(struct qr_file)) != 0)
		if(add_to_qr_list(list, tmp)){
			clear_qr_list(list);
			perror("QR: Unable to load qr_list");
			return;
		}

	close(fd);
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
struct qr_node *search_in_qr(struct qr_node *list, const char *filename){
	struct qr_node *tmpList = list;
	char *base = QR_STOCK;
	struct stat tmp;
	if(stat(strncat(base, filename, strlen(filename)), &tmp) < 0){
		perror("QR: Unable to find the specified file");
		return NULL;
	}  

	ino_t inum = tmp.st_ino;

	while(tmpList){

		ino_t cur_ino = tmpList->data->o_ino.st_ino;

		if(inum == cur_ino) break;
		else if(inum > cur_ino) tmpList = tmpList->right;
		else tmpList = tmpList->left; 
	}

	return tmpList;
	
}

/* Get filename in a path
 * Return the string containing filename on success,
 * NULL on error
 */
char *get_filename(const char *path){
	char sep = '/';
	char *filename;
	int i, l_occ, lg_tot, lg_sub;

  	lg_tot = strlen(path);
  	if(lg_tot <= 0){
  		perror("QR: Path passed in args is null size");
  		return NULL;
  	}
	for(i = 0; i < lg_tot; i++) 
		if(path[i] == sep) 
			l_occ = i;

	if((filename = malloc(lg_tot - i) + 1) == NULL){
		perror("Couldn't allocate memory");
		return NULL;
	}

	for(i = 0; i < l_occ; i++){
		path++;
		lg_sub = i;
	}

	for(i = 0; i < (lg_tot - lg_sub); i++){
		*(filename + i) = *path;
		path++;
	} 

	return filename;
}

/* Recursively write the data in the quarantine tree to QR_DB */
void write_node(struct qr_node **list, const int fd){
	struct qr_node *tmp = *list;
	
	if(!list) return;
	if(tmp->left)  write_node(&tmp->left, fd);
	if(tmp->right) write_node(&tmp->right, fd);

	if(tmp->data != NULL){
		if(write(fd, tmp->data, sizeof(struct qr_file)) < 0){
			perror("Unable to write data from qr_node to QR_DB");
			return;
		}
	}	
}

/* Write the list into the quarantine DB 
 * Return 0 on success and -1 on error
 */
int save_qr_list(struct qr_node **list){
	int fd;

	if(unlink(QR_DB)){
		perror("Unable to remove old");
		return -1;
	}

	if((fd = open(QR_DB, O_WRONLY, S_IRUSR)) < 0){
		perror("Unable to open QR_DB");
		return -1;
	}

	write_node(list, fd);
	clear_qr_list(list);

	close(fd);
	return 0;	
}

/* Move file to STOCK_QR */
void add_file_to_qr(struct qr_node **list, const char *file){
	struct qr_file *new_f = malloc(sizeof(struct qr_file));
	struct stat new_s;
	char *new_path = malloc(strlen(QR_STOCK) + 1);
	char *fn = get_filename(file);

	if((new_f == NULL) || (new_path == NULL )){
		perror("QR: Couldn't allocate memory");
		return;
	}

	if(access(fn, F_OK) != -1){
		char *tmp = malloc(strlen(fn)+(sizeof(char)*4));
		int i = 1;
		do{
			if(snprintf(tmp, strlen(fn) + sizeof(i) +1, "%s%d", fn, i) < 0){
				perror("QR: Unable to increment filename");
				return;
			}
			i++;
		} while(access(tmp, F_OK) != -1);
		if(!strncpy(fn, tmp, strlen(tmp) + 1)){
			perror("QR: Unable to modify filename");
			return;
		}
	}

	if(!strncpy(new_path, QR_STOCK, strlen(QR_STOCK)+1)){
		perror("QR: Unable to create new_path");
		return;
	}

	if(!strncat(new_path, fn, strlen(fn))){
		perror("QR: Unable to concatenate fn in new_path");
		return;
	}

	if(stat(new_path, &new_s) < 0){
		perror("QR: Unable to get stat of file");
		return;
	}

	new_f->o_ino = new_s;
	if(!strncpy(new_f->o_path, file, strlen(file)+1)){
		perror("QR: Unable to put current path to old path");
		return;
	}
	if(!strncpy(new_f->f_name, fn, strlen(fn)+1)){
		perror("QR: Unable to put new filename to qr_file struct");
		return;
	}

	if(new_f->f_name == NULL)
		return;

	new_f->d_begin   = time(NULL);
	/* This will be changed when config implemented 
	 * Choice will be between expire configured and not configured
	 */
	new_f->d_expire  = 0;

        if(rename(file, new_path)){
        	perror("Adding aborted: Unable to move the file");
        	return;
        }

        add_to_qr_list(list, new_f);
}

/* Delete definitively a file from the quarantine */
void rm_file_from_qr(const char *file, struct qr_node **list){
	NOT_YET_IMP;
}

/* Restore file to its anterior state and place */
int restore_file(struct qr_node **list, const char *file)
{
	struct qr_node *res_file = search_in_qr(*list, file);
	if (res_file == NULL) {
		perror("QR ERROR: Unable to find file to restore");
		return -1;
	}
	if (rm_from_qr_list(list, res_file->data->o_ino.st_ino)) {
		perror("QR: Unable to remove file from QR list");
		return -1;
	}
	if (rename(file, res_file->data->o_path)) {
		perror("Restore aborted: Unable to move the file");
		return -1;
	}
	return 0;
}

/* Remove file from the qr_list 
 * Return 0 on success, -1 on error 
 */
int rm_from_qr_list(struct qr_node **list, ino_t i_num){
	NOT_YET_IMP;
	return -1;
}
