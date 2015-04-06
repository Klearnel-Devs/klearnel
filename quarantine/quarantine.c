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
int _add_to_qr_list(QrList **list, QrData new_f)
{
	QrListNode *node = calloc(1, sizeof(QrListNode));
	if (!node) {
		return -1;
	}
	node->data = new_f;
	if((*list)->last == NULL) {
		(*list)->first = node;
		(*list)->last = node;
	} else if ((*list)->last->data.o_ino.st_ino < new_f.o_ino.st_ino) {
		(*list)->last->next = node;
		node->prev = (*list)->last;
		(*list)->last = node;
	} else if ((*list)->first->data.o_ino.st_ino > new_f.o_ino.st_ino) {
		node->next = (*list)->first;
		(*list)->first->prev = node;
		(*list)->first = node;
	} else {
		QrListNode *tmp = (*list)->first;
		QrListNode *tmp_prev = NULL;
		while(tmp->data.o_ino.st_ino < node->data.o_ino.st_ino) {
			tmp_prev = tmp;
			tmp = tmp->next;
		}
		node->next = tmp;
		node->prev = tmp->prev;
		tmp->prev = node;
		tmp_prev->next = node;
	}
	(*list)->count++;
	return 0;
}

QrListNode* _find_QRNode(QrListNode *node, int num) 
{
	if (node == NULL){
		goto out;
	}
	else if(node->data.o_ino.st_ino < num){
		node = _find_QRNode(node->next, num);
	}
	out:
	return node;
}

/* Remove file data from the qr_list */
int _rm_from_qr_list(QrList **list, QrListNode *node)
{
	if(((*list)->first == NULL) && ((*list)->last == NULL)){
		write_to_log(WARNING, "%s:%d - %s", __func__, __LINE__, "List is empty!");
		if (node != NULL)
			free(node);
		goto error;
	} else if (node == NULL){
		write_to_log(WARNING, "%s:%d - %s", __func__, __LINE__, "Node to remove is empty!");
		goto error;
	}
	// Try to free the deleted node?
	if((node->data.o_ino.st_ino == (*list)->first->data.o_ino.st_ino) && (node->data.o_ino.st_ino == (*list)->last->data.o_ino.st_ino)) {
		(*list)->first = NULL;
		(*list)->last = NULL;
		write_to_log(INFO,"%s : %s", "Last file removed from QR List", node->data.f_name);
	} else if(node->data.o_ino.st_ino == (*list)->first->data.o_ino.st_ino) {
		(*list)->first = node->next;
		if((*list)->first == NULL)
			write_to_log(WARNING, "%s:%d - %s", __func__, __LINE__, "Invalid list, somehow got a first that is NULL");
		(*list)->first->prev = NULL;
	} else if (node->data.o_ino.st_ino == (*list)->last->data.o_ino.st_ino) {
		(*list)->last = node->prev;
		if((*list)->last == NULL)
			write_to_log(WARNING, "%s:%d - %s", __func__, __LINE__, "Invalid list, somehow got a tmp_prev that is NULL.");
		(*list)->last->next = NULL;
	} else {
		QrListNode *after = node->next;
		QrListNode *before = node->prev;
		after->prev = before;
		before->next = after;
	}
	(*list)->count--;
	write_to_log(INFO, "%s - %s", "File removed from QR List", node->data.f_name);
	free(node);
	return 0;
	error:
		return -1;
}

/* Recursively write the data in the quarantine list to QR_DB */
void _write_node(QrListNode *listNode, int fd, int custom)
{
	if (listNode==NULL) {
		return;
	}
	if (write(fd, &listNode->data, sizeof(QrData)) < 0) {
		if (custom < 0) {
			write_to_log(URGENT, "%s - %d - %s", __func__, __LINE__, "Unable to write data from qr_node to QR_DB");
		} 
		return;
	}
	if (custom < 0) {
		write_to_log(INFO, "%s - %s", "File written to QR_DB", listNode->data.f_name);
	}
	_write_node(listNode->next, fd, custom);
}

/* Move file to STOCK_QR */
int  add_file_to_qr(QrList **list, char *filepath)
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
	new_f.d_expire  = new_f.d_begin + EXP_DEF;

	if (rename(filepath, new_path)) {
		write_to_log(WARNING, "%s - %d - Adding aborted: Unable to move the file %s to %s", __func__, __LINE__, filepath, new_path);
		goto error;
	}

	if (_add_to_qr_list(list, new_f) != 0){
		write_to_log(FATAL, "%s - %s - %s", __func__, __LINE__, "Unable to allocate memory");
		exit(EXIT_FAILURE);
	} else {
		write_to_log(INFO, "%s - %s", "File successfully added to QR List", new_f.f_name);
	}
		
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
		if (creat(QR_DB, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the database file", QR_DB);
			return;
		}
	}
	write_to_log(INFO, "%s", "QR Initialized without error");
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void clear_qr_list(QrList **list)
{
	if ((*list)->first != NULL) {
		LIST_FOREACH(list, first, next, cur) {
			if (cur->prev) {
			    	free(cur->prev);
			}
		}
	}
	free(*list);
	*list = NULL;
}

/*
 * Load temporary QR file (used by user to list files in qr)
 */
void load_tmp_qr(QrList **list, int fd)
{
	QrData tmp;
	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		if (_add_to_qr_list(list, tmp) != 0){
			perror("Out of memory!");
			exit(EXIT_FAILURE);
		}
	}
}


/* Load quarantine with content of QR_DB  
 */
void load_qr(QrList **list)
{
	int fd;
	QrData tmp;
	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to open QR_DB", QR_DB);
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		if (_add_to_qr_list(list, tmp) != 0){
			write_to_log(FATAL, "%s - %s - %s", __func__, __LINE__, "Unable to allocate memory");
			exit(EXIT_FAILURE);
		}
		write_to_log(INFO, "%s : %s", "Quarantine loading file", tmp.f_name);
	}
	close(fd);
	write_to_log(INFO, "%s", "Quarantine loaded with content of QR_DB");
}


/*
 * Print all elements contained in qr-list to stdout
 */
void print_qr(QrList **list)
{
	clock_t begin, end;
	double spent;
	begin = clock();
	printf("Quarantine elements:\n");
	LIST_FOREACH(list, first, next, cur) {
		printf("\nFile \"%s\":\n", cur->data.f_name);
		printf("\t- Old path: %s\n", cur->data.o_path);
		printf("\t- In QR since %d\n", (int)cur->data.d_begin);
	}
	end = clock();
	spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("\nQuery executed in: %.2lf seconds\n", spent);
}

/* Write the list into the quarantine DB 
 * Can save list to another file than QR_STOCK with param: custom (file descriptor)
 * custom: must be set to -1 if not used
 * Return 0 on success and -1 on error
 */
int save_qr_list(QrList **list, int custom)
{
	int fd;

	if (custom >= 0) {
		fd = custom;
	} else {
		if ((fd = open(QR_DB, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to open QR_DB", QR_DB);
			return -1;
		}
	}
	_write_node((*list)->first, fd, custom);
	if (custom < 0) {
		write_to_log(INFO, "%s", "QR List has been saved");
		close(fd);
	}
	return 0;	
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QrListNode* search_in_qr(QrList *list, char *filename)
{
	char *base;
	QrListNode *tmpNode = NULL;
	struct stat tmp;
	if (!(base = malloc(strlen(QR_STOCK) + strlen(filename) + 5))) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		goto out;
	}
	if (snprintf(base, (strlen(QR_STOCK) + strlen(filename) + 5), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s/%s", __func__, __LINE__, "Unable to create the path of the searched file", QR_STOCK, filename);
		goto out;
	}
	if (stat(base, &tmp) < 0) {
		write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to find the specified file to stat", base);
		goto out;
	}
	if ((tmpNode = _find_QRNode(list->first, tmp.st_ino)) == NULL) {
		write_to_log(URGENT, "%s:%d - %s : %s", "File could not be found in list", filename);
		if (unlink(base)) {
			write_to_log(URGENT, "%s - %d - %s : %s", __func__, __LINE__, "Undetermined. File exists in QR_STOCK but NOT QR_LIST and cannot be unlinked", base);
		}
		goto out;
	}
out:
	free(base);
	return tmpNode;
	
}

/* Delete definitively a file from the quarantine */
int rm_file_from_qr(QrList **list, char *filename)
{
	QrListNode *rm_file;
	char *p_rm = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if ((p_rm ) == NULL) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}
	if (snprintf(p_rm, (strlen(QR_STOCK)+strlen(filename)+3), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s : %s/%s", __func__, __LINE__, "Unable to create path to file to remove", QR_STOCK, filename);
		goto err;
	}
	if ((rm_file = search_in_qr(*list, filename)) == NULL){
		write_to_log(WARNING, "%s - %d - %s : %s", __func__, __LINE__, "Unable to locate file to remove", filename);
		goto err;
	}
	if (_rm_from_qr_list(list, rm_file) != 0) {
		goto err;
	}
	if (unlink(p_rm)) {
		write_to_log(URGENT, "%s - %d - %s : %s", __func__, __LINE__, "Unable to remove file from stock, list will not be saved", p_rm);
		goto err;
	}
	if (save_qr_list(list, -1) == 0) {
		write_to_log(INFO, "File %s removed from QR", p_rm);
	}
	free(p_rm);
	return 0;
err:
	free(p_rm);
	return -1;
}

/* Restore file to its anterior state and place */
int restore_file(QrList **list, char *filename)
{
	QrListNode *res_file = search_in_qr(*list, filename);
	char *p_res = malloc(strlen(QR_STOCK)+strlen(filename)+3);
	if (!p_res) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}
	if (res_file == NULL) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to find file to restore");
		goto out;
	}

	if (_rm_from_qr_list(list, res_file) != 0) {
		goto out;
	}

	if (snprintf(p_res, (strlen(QR_STOCK)+strlen(filename)+3), "%s/%s", QR_STOCK, filename) < 0) {
		write_to_log(WARNING, "%s - %d - %s %s/%s", __func__, __LINE__, "Unable to create path to remove", QR_STOCK, filename);
		goto out;
	}

	if (rename(p_res, res_file->data.o_path) != 0) {
		write_to_log(URGENT, "%s - %d - %s - %s to %s", __func__, __LINE__, "Restore aborted: Unable to move the file ", filename, res_file->data.o_path);
		load_qr(list);
		goto out;
	}
	if (save_qr_list(list, -1) == 0) {
		write_to_log(INFO, "File %s removed from QR and restored to %s", filename, res_file->data.o_path);
	}
	
	free(p_res);
	return 0;
out:
	free(p_res);
	return -1;
}