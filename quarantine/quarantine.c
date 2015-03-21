/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>

/* Add a file to the quarantine list 
 */
void _add_to_qr_list(QRList list, QrData new_f)
{
	QRListNode *node = calloc(1, sizeof(QRListNode));
	if (!node) {
		write_to_log(FATAL, "%s - %s - %s", __func__, __LINE__, "Unable to allocate memory");
		exit(EXIT_FAILURE);
	}
	check_mem(node);

	node->data = new_f;

	if(list->last == NULL) {
		list->first = node;
		list->last = node;
	} else if (list->last->data.o_ino.st_ino < new_f.o_ino.st_ino) {
		list->last->next = node;
		node->prev = list->last;
		list->last = node;
	} else if (list->first->data.o_ino.st_ino > new_f.o_ino.st_ino) {
		node->next = list->first;
		list->first->prev = node;
		list->first = node;
	} else {
		QRListNode tmp = list->first;
		QRListNode tmp_prev = NULL;
		while(tmp->data.o_ino.st_ino < node->data.o_ino.st_ino) {
			tmp_prev = tmp;
			tmp->next;
		}
		node->next = tmp;
		node->prev = tmp->prev;
		tmp->prev = node;
		tmp_prev->next = node;
	}
	list->count++;
	write_to_log(INFO, "%s - %s", "File successfully added to QR List (left)", list->data.f_name);
	return;
}

/* Remove file data from the qr_list */
int _rm_from_qr_list(QRList list, QRListNode node)
{
	if((list->first == NULL) && (list->last == NULL)){
		write_to_log
		goto error;
	} else if (node == NULL){
		write_to_log
		goto error;
	}

	if((node.o_ino.st_ino == list->first->data.o_ino.st_ino) && (node == list->last->data.o_ino.st_ino)) {
		list->first = NULL;
		list->last = NULL;
		write_to_log
	} else if(node.o_ino.st_ino == list->first->data.o_ino.st_ino) {
		list->first = node->next;
		if(list->first != NULL)
			write_to_log("Invalid list, somehow got a first that is NULL.");
		list->first->prev = NULL;
	} else if (node.o_ino.st_ino == list->last->data.o_ino.st_ino) {
		list->last = node->prev;
		if(list->last != NULL)
			write_to_log("Invalid list, somehow got a next that is NULL.");
		list->last->next = NULL;
	} else {
		QRListNode *after = node->next;
		QRListNode *before = node->prev;
		after->prev = before;
		before->next = after;
	}
	list->count--;
	free(node);
	write_to_log(INFO, "%s - %s", "File removed from QR List", node_rm.f_name);
	return 0;
	error:
		return -1;
}

/* Recursively write the data in the quarantine tree to QR_DB */
void _write_node(QRListNode listNode, int fd)
{
	if (listNode==NULL) {
		return;
	}


	if (write(fd, &listNode->data, sizeof(QrData)) < 0) {
		write_to_log(URGENT, "%s - %d - %s", __func__, __LINE__, "Unable to write data from qr_node to QR_DB");
		return;
	}
	_write_node(list->next, fd);

	write_to_log(INFO, "%s - %s", "File written to QR_DB", list->data.f_name);
}

void _print_node(QRListNode node) 
{
	if (node == NULL) return;
	printf("\nFile \"%s\":\n", node->data.f_name);
	printf("\t- Old path: %s\n", node->data.o_path);
	printf("\t- In QR since %d\n", (int)node->data.d_begin);

	_print_node(node->next);
}

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

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void clear_qr_list(QRList list)
{
	LIST_FOREACH(list, first, next, cur) {
		if(cur->prev) {
		    	free(cur->prev);
		}
	}

	free(list->last);
	free(list);
}

/*
 * Load temporary QR file (used by user to list files in qr)
 */
void load_tmp_qr(QRList *list, int fd)
{
	list = calloc(1, sizeof(QRList));
	QrData tmp;
	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		_add_to_qr_list(list, tmp);
		write_to_log(INFO, "%s : %s", "Quarantine loading file", tmp.f_name);
	}
}


/* Load quarantine with content of QR_DB  
 */
void load_qr(QRList *list)
{
	list = calloc(1, sizeof(QRList));
	int fd;
	QrData tmp;
	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to open QR_DB", QR_DB);
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		_add_to_qr_list(list, tmp);
		write_to_log(INFO, "%s : %s", "Quarantine loading file", tmp.f_name);
	}
	close(fd);
	write_to_log(INFO, "%s", "Quarantine loaded with content of QR_DB");
}

/* Write the list into the quarantine DB 
 * Can save list to another file than QR_STOCK with param: custom (file descriptor)
 * custom: must be set to -1 if not used
 * Return 0 on success and -1 on error
 */
int save_qr_list(QRList *list, int custom)
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
	_write_node(list->first, fd);
	write_to_log(INFO, "%s", "QR List has been saved");
	if (custom < 0) 
		close(fd);
	clear_qr_list(list);
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

	_add_to_qr_list(list, new_f);
		
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

QRListNode _find_QRNode(QRListNode node, int num) 
{
	if (node == NULL){
		return NULL;
	}
	else if(node->data.o_ino.st_ino < num){
		node = _find_QRNode(node->next, num);
	}
	return node;
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QRListNode search_in_qr(QRList *list, char *filename)
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
	if ((QRListNode tmp = _find_QRNode(list->first, tmp.st_ino)) == NULL) {
		return NULL;
	}

out:
	free(base);
	return tmp;
	
}

/* Delete definitively a file from the quarantine */
int rm_file_from_qr(QrSearchTree *list, char *filename)
{
	QRListNode rm_file;
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
	_rm_from_qr_list(list, rm_file->data);
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
int restore_file(QRList *list, char *filename)
{
//	QrPosition res_file = search_in_qr(*list, filename);
	char *p_res = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if (!p_res) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}
	if (res_file == NULL) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to find file to restore");
		return -1;
	}

	_rm_from_qr_list(list, res_file->data);

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