/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>

/* Initialize all requirements for Quarantine */
void init_qr()
{
	if (access(QR_STOCK, F_OK) == -1) {
		if (mkdir(QR_STOCK, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the stock", QR_STOCK);
			return;
		}
	}
	if (access(QR_DB, F_OK) == -1) {
		if (creat(QR_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the database file", QR_DB);
			return;
		}
	}
	write_to_log(INFO, "%s", "QR Initialized without error");
}

/* Add a file to the quarantine list 
 * Return 0 on success, -1 on error
 */
QrSearchTree _add_to_qr_list(QrSearchTree list, QrData new_f)
{
	if (list == NULL)  {
		write_to_log(INFO, "%s : %s", "List is empty, loading root", new_f.f_name);
		QrSearchTree new_n = (QrSearchTree) malloc(sizeof(struct qr_node));
		if (!new_n) {
			write_to_log(FATAL, "%s - %s - %s", __func__, __LINE__, "Unable to allocate memory");
			exit(EXIT_FAILURE);
		}
		new_n->data = new_f;
		new_n->left = NULL;
		new_n->right = NULL;
		list = new_n;
	} else if (new_f.o_ino.st_ino < list->data.o_ino.st_ino) {
		list->left = _add_to_qr_list(list->left, new_f);
		write_to_log(INFO, "%s - %s", "File successfully added to QR List (left)", list->data.f_name);
	} else if (new_f.o_ino.st_ino > list->data.o_ino.st_ino) {
		list->right = _add_to_qr_list(list->right, new_f);
		write_to_log(INFO, "%s - %s", "File successfully added to QR List (right)", list->data.f_name);
	}
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

/*
 * Load temporary QR file (used by user to list files in qr)
 */
void load_tmp_qr(QrSearchTree *list, int fd)
{
	QrData tmp;
	while (read(fd, &tmp, sizeof(struct qr_file)) != 0)
		*list = _add_to_qr_list(*list, tmp);
}


/* Load quarantine with content of QR_DB  
 */
void load_qr(QrSearchTree *list)
{
	int fd;
	QrData tmp;
	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to open QR_DB", QR_DB);
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		write_to_log(INFO, "%s : %s", "Quarantine loading file", tmp.f_name);
		*list = _add_to_qr_list(*list, tmp);
	}
	close(fd);
	write_to_log(INFO, "%s", "Quarantine loaded with content of QR_DB");
}

/*
 * Search for min node in right branch
 */
QrPosition _find_min(QrSearchTree list)
{
	if (list == NULL) {
                return NULL;
	} else {
		if (list->left == NULL) {
			return list;
		} else {
			return _find_min(list->left);
		}
	}
}

QrPosition _find_max(QrSearchTree list)
        {
            if (list != NULL)
                while( list->right != NULL ) {
                    list = list->right;
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
		list->left = _rm_from_qr_list(list, node_rm);
	} else if (node_rm.o_ino.st_ino > list->data.o_ino.st_ino){
		list->right = _rm_from_qr_list(list, node_rm);
	} else if ((list->left != NULL) && (list->right != NULL)){
		tmp = _find_min(list->right);
		list->data = tmp->data;
		list->right = _rm_from_qr_list(list->right, list->data);
	} else {
		tmp = list;
		if (list->left == NULL)
			list = list->right;
		else if (list->right == NULL)
			list = list->left;
		free(tmp);
	}
	write_to_log(INFO, "%s - %s", "File removed from QR List", node_rm.f_name);
	return list;
}

QrPosition _find(QrSearchTree list, ino_t inum) {
	if (list == NULL) {
		write_to_log(DEBUG, "%s - %s", "File searched in QR not found", list->data.f_name);
		return NULL;
	}
	if (inum < list->data.o_ino.st_ino)
		return _find(list->left, inum);
	else if (inum > list->data.o_ino.st_ino)
		return _find(list->right, inum);
	else {
		write_to_log(DEBUG, "%s - %s", "File searched in QR found", list->data.f_name);
		return list;
	}
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QrPosition search_in_qr(QrSearchTree list, char *filename)
{
	char *base;
	struct stat tmp;
	if (!(base = malloc(strlen(QR_STOCK) + strlen(filename) + 5))) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return NULL;
	}
	if (snprintf(base, (strlen(QR_STOCK) + strlen(filename) + 5), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s/%s", __func__, __LINE__, "Unable to create the path of the searched file", QR_STOCK, filename);
		goto out;
	}
	if (stat(base, &tmp) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to find the specified file to stat", base);
		goto out;
	}
	ino_t inum = tmp.st_ino;
	list = _find(list, inum);
out:
	free(base);
	return list;
	
}

/* Recursively write the data in the quarantine tree to QR_DB */
void _write_node(QrSearchTree list, int fd)
{
	if (list==NULL) {
		return;
	}
	_write_node(list->left, fd);
	_write_node(list->right, fd);

	if (write(fd, &list->data, sizeof(QrData)) < 0) {
		write_to_log(URGENT, "%s - %d - %s", __func__, __LINE__, "Unable to write data from qr_node to QR_DB");
		return;
	}
	write_to_log(INFO, "%s - %s", "File written to QR_DB", list->data.f_name);
}

/* Write the list into the quarantine DB 
 * Can save list to another file than QR_STOCK with param: custom (file descriptor)
 * custom: must be set to -1 if not used
 * Return 0 on success and -1 on error
 */
int save_qr_list(QrSearchTree *list, int custom)
{
	int fd;

	if (custom >= 0) {
		fd = custom;
	} else {
		if ((fd = open(QR_DB, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to open QR_DB", QR_DB);
			return -1;
		}
	}
	_write_node(*list, fd);
	write_to_log(INFO, "%s", "QR List has been saved");
	if (custom < 0) 
		close(fd);
	*list = clear_qr_list(*list);
	write_to_log(INFO, "%s", "Quarantine list cleared");
	
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
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Couldn't allocate memory");
        goto error;
	}

	if (access(fn, F_OK) != -1) {		
		do {
			if (snprintf(tmp, strlen(fn) + sizeof(i) +1, "%s%d", fn, i) < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to increment filename");
        		goto error;
			}
			i++;
		} while(access(tmp, F_OK) != -1);
		if (!strncpy(fn, tmp, strlen(tmp) + 1)) {
			write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to modify filename");
        	goto error;
		}
	}

	if (!snprintf(new_path, (strlen(QR_STOCK)+strlen(fn)+10), "%s/%s", QR_STOCK, fn) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s/%s", __func__, __LINE__, "Unable to create the new path for file added", QR_STOCK, fn);
		goto error;
	}

	if (stat(filepath, &new_s) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to get stat of file", filepath);
		goto error;
	}

	new_f.o_ino = new_s;
	if (!strncpy(new_f.o_path, filepath, strlen(filepath)+1)) {
		write_to_log(WARNING, "%s - %d - Unable to put current path - %s - to old path - %s", __func__, __LINE__, new_f.o_path, filepath);
        goto error;
	}
	if (!strncpy(new_f.f_name, fn, strlen(fn)+1)) {
		write_to_log(WARNING, "%s - %d - Unable to put new filename - %s - to qr_file struct - %s", __func__, __LINE__, fn, new_f.f_name);
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
		write_to_log(WARNING, "%s - %d - Adding aborted: Unable to move the file %s to %s", __func__, __LINE__, filepath, new_path);
		goto error;
	}

	*list = _add_to_qr_list(*list, new_f);
		
	if (save_qr_list(list, -1) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to save the quarantine in db");
		goto error;
	}
	write_to_log(INFO, "File successfully moved from %s to %s in QR Stock", filepath, new_path);
	free(tmp);
	free(new_path);
	free(cp_path);
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
	char *p_rm = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if ((p_rm ) == NULL) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}
	if (snprintf(p_rm, (strlen(QR_STOCK)+strlen(filename)+3), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s : %s/%s", __func__, __LINE__, "Unable to create path to file to remove", QR_STOCK, filename);
		goto rm_err;
	}
	if ((rm_file = search_in_qr(*list, filename)) == NULL){
		write_to_log(WARNING, "%s - %d - %s : %s", __func__, __LINE__, "Unable to locate file to remove", filename);
		goto rm_err;
	}
	*list = _rm_from_qr_list(*list, rm_file->data);
	if (unlink(p_rm)) {
		write_to_log(URGENT, "%s - %d - %s : %s", __func__, __LINE__, "Unable to remove file from stock, list will not be saved", p_rm);
		goto err;
	}
	save_qr_list(list, -1);
	write_to_log(INFO, "File %s removed from QR", p_rm);
	free(p_rm);
	return 0;
rm_err:
	free(p_rm);
	return -1;
err:
	free(p_rm);
	load_qr(list);
	return -1;
}

/* Restore file to its anterior state and place */
int restore_file(QrSearchTree *list, char *filename)
{
	QrPosition res_file = search_in_qr(*list, filename);
	char *p_res = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if (!p_res) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}
	if (res_file == NULL) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to find file to restore");
		return -1;
	}

	*list = _rm_from_qr_list(*list, res_file->data);

	if (snprintf(p_res, (strlen(QR_STOCK)+strlen(filename)+3), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s %s/%s", __func__, __LINE__, "Unable to create path to remove", QR_STOCK, filename);
		free(p_res);
		return -1;
	}
	if (rename(p_res, res_file->data.o_path))
		write_to_log(URGENT, "%s - %d - %s - %s to %s", __func__, __LINE__, "Restore aborted: Unable to move the file ", filename, res_file->data.o_path);
	save_qr_list(list, -1);
	write_to_log(INFO, "File %s removed from QR and restored to %s", filename, res_file->data.o_path);
	free(p_res);
	return 0;
}

void _print_node(QrSearchTree node) 
{
	if (node == NULL) return;
	printf("\nFile \"%s\":\n", node->data.f_name);
	printf("\t- Old path: %s\n", node->data.o_path);
	printf("\t- In QR since %d\n", (int)node->data.d_begin);

	_print_node(node->left);
	_print_node(node->right);
}

/*
 * Print all elements contained in qr-list to stdout
 */
void print_qr(QrSearchTree list)
{
	clock_t begin, end;
	double spent;
	begin = clock();
	printf("Quarantine elements:\n");
	_print_node(list);
	end = clock();
	spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("\nQuery executed in: %.2lf seconds\n", spent);
}