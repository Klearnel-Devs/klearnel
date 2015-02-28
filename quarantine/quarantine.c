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
QrSearchTree _add_to_qr_list(QrSearchTree list, QrData new_f)
{
	if (list == NULL)  {
		QrSearchTree new = (QrSearchTree) malloc(sizeof(struct qr_node));
		if (!new) {
			perror("[QR] Unable to allocate memory");
			exit(EXIT_FAILURE);
		}
		new->data = new_f;
		new->left = NULL;
		new->right = NULL;
		list = new;
	} else if (new_f.o_ino.st_ino < list->data.o_ino.st_ino) {
		list->left = _add_to_qr_list(list->left, new_f);
	} else if (new_f.o_ino.st_ino > list->data.o_ino.st_ino) {
		list->right = _add_to_qr_list(list->right, new_f);
	} else ;
	return list;
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
QrSearchTree clear_qr_list(QrSearchTree list)
{
	if (list != NULL) {
		clear_qr_list(list->left);
		clear_qr_list(list->right);
		free(list);
	}
	return NULL;
}

/* Load quarantine with content of QR_DB  
 */
void load_qr(QrSearchTree *list)
{
	int fd;
	QrData tmp;
	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		perror("[QR] Unable to open QR_DB");
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) 
		*list = _add_to_qr_list(*list, tmp);
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
QrSearchTree _rm_from_qr_list(QrSearchTree list, QrData node_rm)
{
	QrPosition tmp;
	if (list == NULL) {
		return NULL;
	} else if (node_rm.o_ino.st_ino < list->data.o_ino.st_ino) {
		list->left = _rm_from_qr_list(list->left, node_rm);
	} else if (node_rm.o_ino.st_ino > list->data.o_ino.st_ino) {
		list->right = _rm_from_qr_list(list->right, node_rm);
	} else if ((list->left != NULL) && (list->right != NULL)) {
		tmp = _find_min(list->right);
		list->data = tmp->data;
		list->right = _rm_from_qr_list(list->right, list->data);
	} else {
		tmp = list;
		if (list->left == NULL)
			list = list->right;
		else if (list->right == NULL)
			list = list->left;
	}
	return list;
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QrPosition search_in_qr(QrSearchTree list, char *filename)
{
	char *base;
	struct stat tmp;
	if (!(base = malloc(strlen(QR_STOCK) + strlen(filename) + 5))) {
		perror("[QR] Unable to allocate memory");
		return NULL;
	}
	if (snprintf(base, (strlen(QR_STOCK) + strlen(filename) + 5), "%s/%s", 
		QR_STOCK, filename) < 0) {
		perror("[QR] Unable to create the path of the researched file");
		goto out;
	}
	if (stat(base, &tmp) < 0) {
		perror("[QR] Unable to find the specified file");
		LOG_DEBUG;
		goto out;
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
out:
	free(base);
	return NULL;
	
}

/* Recursively write the data in the quarantine tree to QR_DB */
void _write_node(QrSearchTree list, int fd)
{
	if (!list) return;
	printf("%s: Filename is \"%s\"\n", __func__, list->data.f_name);
	_write_node(list->left, fd);
	_write_node(list->right, fd);

	if (write(fd, &list->data, sizeof(QrData)) < 0) {
		perror("[QR] Unable to write data from qr_node to QR_DB");
		return;
	}
}

/* Write the list into the quarantine DB 
 * Return 0 on success and -1 on error
 */
int save_qr_list(QrSearchTree *list)
{
	int fd;

	if ((fd = open(QR_DB, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		perror("[QR] Unable to open QR_DB");
		return -1;
	}
	_write_node(*list, fd);
	close(fd);
	*list = clear_qr_list(*list);
	return 0;	
}

/* Move file to STOCK_QR */
int  add_file_to_qr(QrSearchTree *list, char *filepath)
{
	QrData new_f;
	struct stat new_s;
	char *cp_path = malloc(strlen(filepath)+1);
	strncpy(cp_path, filepath, strlen(filepath)+1);
	char *fn = basename(cp_path);
	char *new_path = malloc(strlen(QR_STOCK)+strlen(fn)+10);
	char *tmp = malloc(strlen(fn)+4);
	int i = 1;
	
	if ((!new_path) || (!tmp) || (!cp_path)) {
		perror("QR: Couldn't allocate memory");
        	goto error;
	}

	if (access(fn, F_OK) != -1) {		
		do {
			if (snprintf(tmp, strlen(fn) + sizeof(i) +1, "%s%d", fn, i) < 0) {
				perror("QR: Unable to increment filename");
        			goto error;
			}
			i++;
		} while(access(tmp, F_OK) != -1);
		if (!strncpy(fn, tmp, strlen(tmp) + 1)) {
			perror("QR: Unable to modify filename");
        		goto error;
		}
	}

	if (!snprintf(new_path, (strlen(QR_STOCK)+strlen(fn)+10), 
		"%s/%s", QR_STOCK, fn) < 0) {
		perror("[QR] Unable to create the new path for file added");
		goto error;
	}

	if (stat(filepath, &new_s) < 0) {
		perror("QR: Unable to get stat of file");
		goto error;
	}

	new_f.o_ino = new_s;
	if (!strncpy(new_f.o_path, filepath, strlen(filepath)+1)) {
		perror("QR: Unable to put current path to old path");
        	goto error;
	}
	if (!strncpy(new_f.f_name, fn, strlen(fn)+1)) {
		perror("QR: Unable to put new filename to qr_file struct");
        	goto error;
	}

	if (new_f.f_name == NULL)
        	goto error;

	new_f.d_begin   = time(NULL);
	/* This will be changed when config implemented 
	 * Choice will be between expire configured and not configured
	 */
	new_f.d_expire  = 0;

        if (rename(filepath, new_path)) {
        	perror("Adding aborted: Unable to move the file");
        	goto error;
        }

        *list = _add_to_qr_list(*list, new_f);

     	
        if (save_qr_list(list) < 0) {
        	perror("[QR] Unable to save the quarantine in db");
        	goto error;
        }
        free(tmp);
 	free(new_path);
 	free(cp_path);
 	LOG_DEBUG;
        return 0;
error:
	free(tmp);
	free(new_path);
	free(cp_path);
	return -1;
}

/* Delete definitively a file from the quarantine */
int rm_file_from_qr(QrSearchTree *list, char *filename)
{
	QrPosition rm_file;
	char *p_rm;
	if ((p_rm = malloc(strlen(QR_STOCK)+strlen(filename)+3)) == NULL) {
		perror("[QR] Unable to allocate memory");
		return -1;
	}
	if (snprintf(p_rm, (strlen(QR_STOCK)+strlen(filename)+3), 
		"%s/%s", QR_STOCK, filename) < 0) {
		perror("[QR] Unable to create path to file to remove");
		goto rm_err;
	}
	if ((rm_file = search_in_qr(*list, filename)) == NULL){
		perror("QR: Unable to locate file to remove");
		goto rm_err;
	}
	*list = _rm_from_qr_list(*list, rm_file->data);
	if (unlink(p_rm)) {
		perror("QR: Unable to remove file from stock");
		fprintf(stderr, "WARNING: [QR] File has been removed from qr_list!\n");
	}
	save_qr_list(list);
	free(p_rm);
	return 0;
rm_err:
	free(p_rm);
	return -1;
}

/* Restore file to its anterior state and place */
int restore_file(QrSearchTree *list, char *filename)
{
	QrPosition res_file = search_in_qr(*list, filename);
	char *p_rm = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if (!p_rm) {
		perror("[QR] Unable to allocate memory");
		return -1;
	}
	if (res_file == NULL) {
		perror("QR ERROR: Unable to find file to restore");
		return -1;
	}

	*list = _rm_from_qr_list(*list, res_file->data);

	if (snprintf(p_rm, (strlen(QR_STOCK)+strlen(filename)+3), 
		"%s/%s", QR_STOCK, filename) < 0) {
		perror("QR: Unable to create path for file to remove");
		free(p_rm);
		return -1;
	}
	if (rename(p_rm, res_file->data.o_path))
		perror("Restore aborted: Unable to move the file");
	save_qr_list(list);
	free(p_rm);
	return 0;
}
