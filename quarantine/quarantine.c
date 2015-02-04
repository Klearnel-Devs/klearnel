/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>

/* Initialize all requirements for Quarantine */
void init_qr()
{
	if (access(QR_STOCK, F_OK) == -1) {
		if (mkdir(QR_STOCK, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			perror("QR: Unable to create the stock");
			return;
		}
	}
	if (access(QR_DB, F_OK) == -1) {
		if (creat(QR_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			perror("QR: Unable to create the database file");
			return;
		}
	}
}

/* Add a file to the quarantine list 
 * Return 0 on success, -1 on error
 */
void add_to_qr_list(QrSearchTree *list, QrData new_f)
{
	if (*list == NULL)  {
		QrSearchTree new = (QrSearchTree) malloc(sizeof(struct qr_node));
		if (!new) {
			perror("[QR] Unable to allocate memory");
			exit(EXIT_FAILURE);
		}
		new->data = new_f;
		new->left = NULL;
		new->right = NULL;
		*list = new;
	} else if (new_f.o_ino.st_ino < (*list)->data.o_ino.st_ino) {
		add_to_qr_list(&(*list)->left, new_f);
	} else if (new_f.o_ino.st_ino > (*list)->data.o_ino.st_ino) {
		add_to_qr_list(&(*list)->right, new_f);
	} 
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void clear_qr_list(QrSearchTree *list)
{
	DEBUG_NOTIF;
	if (list != NULL) {
		clear_qr_list(&(*list)->left);
		clear_qr_list(&(*list)->right);
		free(*list);
	}
}

/* Load quarantine with content of QR_DB  
 */
void load_qr(QrSearchTree *list)
{
	int fd;
	QrData tmp;
	DEBUG_NOTIF;
	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		perror("[QR] Unable to open QR_DB");
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) 
		add_to_qr_list(list, tmp);
	DEBUG_NOTIF;
	close(fd);
}

/*
 * Search for min node in right branch
 */
QrPosition _find_min(QrSearchTree list)
{
	if (list != NULL){
		while (list->left != NULL)
			list = list->left;
	}
	return list;
}

/* Remove file data from the qr_list */
void _rm_from_qr_list(QrSearchTree *list, QrData node_rm)
{
	QrPosition tmp;
	if ((*list) == NULL) {
		perror("QR: Quarantine BST is empty!");
		return;
	} else if (node_rm.o_ino.st_ino < (*list)->data.o_ino.st_ino) {
		_rm_from_qr_list(&(*list)->left, node_rm);
	} else if (node_rm.o_ino.st_ino > (*list)->data.o_ino.st_ino) {
		_rm_from_qr_list(&(*list)->right, node_rm);
	} else if (((*list)->left != NULL) && ((*list)->right != NULL)) {
		tmp = _find_min((*list)->right);
		(*list)->data = tmp->data;
		_rm_from_qr_list(&(*list)->right, (*list)->data);
	} else {
		tmp = (*list);
		if ((*list)->left == NULL)
			(*list) = (*list)->right;
		else if ((*list)->right == NULL)
			(*list) = (*list)->left;
		free(tmp);
	}
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QrPosition search_in_qr(QrSearchTree list, char *filename)
{
	char *base;
	struct stat tmp;
	if ((base = malloc(strlen(QR_STOCK) + 1)) == NULL) {
		perror("[QR] Unable to allocate memory");
		return NULL;
	}
	if (!strncpy(base, QR_STOCK, strlen(QR_STOCK) + 1)) {
		perror("[QR] Unable to create QR_STOCK base path");
		return NULL;
	}
	if (strncat(base, filename, strlen(base)) == NULL) {
		perror("[QR] Unable to concatenate filepath");
		return NULL;
	}
	if (stat(base, &tmp) < 0) {
		perror("QR: Unable to find the specified file");
		return NULL;
	}

	ino_t inum = tmp.st_ino;

	while (list) {

		ino_t cur_ino = list->data.o_ino.st_ino;

		if (inum == cur_ino) {
			return list;	
		} else if (inum > cur_ino) {
			list = list->right;
		} else {
			list = list->left;
		} 
	}
	return NULL;
	
}

/* Get filename in a path
 * Return the string containing filename on success,
 * NULL on error
 */
char *_get_filename(const char *path)
{
	char sep = '/';
	char *filename;
	int i, l_occ, lg_tot, lg_sub;

  	lg_tot = strlen(path);
  	if (lg_tot <= 0) {
  		perror("QR: Path passed in args is null size");
  		return NULL;
  	}

	for (i = 0; i < lg_tot; i++) 
		if(path[i] == sep) 
			l_occ = i;

	if ((filename = malloc(lg_tot - i) + 1) == NULL) {
		perror("Couldn't allocate memory");
		return NULL;
	}

	for (i = 0; i < l_occ; i++) {
		path++;
		lg_sub = i;
	}

	for (i = 0; i < (lg_tot - lg_sub); i++) {
		*(filename + i) = *path;
		path++;
	} 

	return filename;
}

/* Recursively write the data in the quarantine tree to QR_DB */
void _write_node(QrSearchTree list, const int fd)
{
	
	if (!list) return;
	_write_node(list->left, fd);
	_write_node(list->right, fd);

	if (list->data.f_name != NULL) {
		if (write(fd, &list->data, sizeof(struct qr_file)) < 0) {
			perror("Unable to write data from qr_node to QR_DB");
			return;
		}
	}	
}

/* Write the list into the quarantine DB 
 * Return 0 on success and -1 on error
 */
int save_qr_list(QrSearchTree *list)
{
	int fd;

	if (unlink(QR_DB)) {
		perror("[QR] Unable to remove old");
		return -1;
	}

	if (creat(QR_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
		perror("QR: Unable to create the database file");
		return -1;
	}

	if ((fd = open(QR_DB, O_WRONLY, S_IWUSR)) < 0) {
		perror("[QR] Unable to open QR_DB");
		return -1;
	}

	_write_node(*list, fd);
	clear_qr_list(list);

	close(fd);
	return 0;	
}

/* Move file to STOCK_QR */
int  add_file_to_qr(QrSearchTree *list, const char *filepath)
{
	QrData new_f;
	struct stat new_s;
	char *new_path = malloc(strlen(QR_STOCK) + 1);
	char *fn = _get_filename(filepath);
	DEBUG_NOTIF;
	if ((new_path == NULL )) {
		perror("QR: Couldn't allocate memory");
        	return -1;
	}

	if (access(fn, F_OK) != -1) {
		char *tmp = malloc(strlen(fn)+(sizeof(char)*4));
		int i = 1;
		do {
			if (snprintf(tmp, strlen(fn) + sizeof(i) +1, "%s%d", fn, i) < 0) {
				perror("QR: Unable to increment filename");
        			return -1;
			}
			i++;
		} while(access(tmp, F_OK) != -1);
		if (!strncpy(fn, tmp, strlen(tmp) + 1)) {
			perror("QR: Unable to modify filename");
        		return -1;
		}
	}

	if (!strncpy(new_path, QR_STOCK, strlen(QR_STOCK)+1)) {
		perror("QR: Unable to create new_path");
		return -1;
	}

	if (!strncat(new_path, fn, strlen(fn))) {
		perror("QR: Unable to concatenate fn in new_path");
		return -1;
	}

	if (stat(filepath, &new_s) < 0) {
		perror("QR: Unable to get stat of file");
		return -1;
	}

	new_f.o_ino = new_s;
	if (!strncpy(new_f.o_path, filepath, strlen(filepath)+1)) {
		perror("QR: Unable to put current path to old path");
        	return -1;
	}
	if (!strncpy(new_f.f_name, fn, strlen(fn)+1)) {
		perror("QR: Unable to put new filename to qr_file struct");
        	return -1;
	}

	if (new_f.f_name == NULL)
        	return -1;

	new_f.d_begin   = time(NULL);
	/* This will be changed when config implemented 
	 * Choice will be between expire configured and not configured
	 */
	new_f.d_expire  = 0;

        if (rename(filepath, new_path)) {
        	perror("Adding aborted: Unable to move the file");
        	return -1;
        }

        add_to_qr_list(list, new_f);

     	DEBUG_NOTIF;
        if (save_qr_list(list) < 0) {
        	perror("[QR] Unable to save the quarantine in db");
        	return -1;
        }
 
        return 0;
}

/* Delete definitively a file from the quarantine */
int rm_file_from_qr(QrSearchTree *list, char *filename)
{
	QrPosition rm_file;
	char *p_rm;
	if ((p_rm = malloc(strlen(QR_STOCK) + 1)) == NULL) {
		perror("[QR] Unable to allocate memory");
		return -1;
	}
	if (!strncpy(p_rm, QR_STOCK, strlen(QR_STOCK) + 1)) {
		perror("[QR] Unable to create QR_STOCK base path");
		return -1;
	}
	if (strncat(p_rm, filename, strlen(filename)) == NULL) {
		perror("QR: Unable to create path for file to remove");
		return -1;
	}
	DEBUG_NOTIF;
	if ((rm_file = search_in_qr(*list, filename)) == NULL){
		perror("QR: Unable to locate file to remove");
		return -1;
	}
	_rm_from_qr_list(list, rm_file->data);
	if (unlink(p_rm)) {
		perror("QR: Unable to remove file from stock");
		fprintf(stderr, "WARNING: [QR] File has been removed from qr_list!\n");
	}
	save_qr_list(list);
	return 0;
}

/* Restore file to its anterior state and place */
int restore_file(QrSearchTree *list, char *filename)
{
	QrPosition res_file = search_in_qr(*list, filename);
	char *p_rm = QR_STOCK;
	if (res_file == NULL) {
		perror("QR ERROR: Unable to find file to restore");
		return -1;
	}
	_rm_from_qr_list(list, res_file->data);
	if (strncat(p_rm, filename, strlen(filename)) == NULL) {
		perror("QR: Unable to create path for file to remove");
		return -1;
	}
	if (rename(p_rm, res_file->data.o_path))
		perror("Restore aborted: Unable to move the file");
	save_qr_list(list);
	return 0;
}
