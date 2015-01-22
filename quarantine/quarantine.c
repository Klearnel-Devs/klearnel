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
		if (creat(QR_DB, S_IRWXU | S_IRGRP | S_IROTH)) {
			perror("QR: Unable to create the database file");
			return;
		}
	}
}

/* Init a node of the quarantine with all attributes at NULL 
 * Return 0 on success, -1 on error
 */
QrSearchTree _new_qr_node()
{
	QrSearchTree new_node = (QrSearchTree)malloc(sizeof(struct qr_node));
	if (new_node == NULL) {
		perror("Couldn't allocate memory");
		goto out;
	}
	new_node->left = NULL;
	new_node->right = NULL;
out:
	return new_node;
}

/* Add a file to the quarantine list 
 * Return 0 on success, -1 on error
 */
int add_to_qr_list(QrSearchTree list, QrData new_f)
{
	QrSearchTree tmpNode;
	QrSearchTree tmpList = list;
	QrSearchTree elem;

	if ((elem = _new_qr_node()) == NULL) {
		perror("QR: Unable to create new qr node");
		return -1;
	}

	elem->data = new_f;

	if (tmpList) {
		do {
			tmpNode = tmpList;
			if (new_f.o_ino.st_ino > tmpList->data.o_ino.st_ino) {
		            	tmpList = tmpList->right;
		            	if (!tmpList) tmpNode->right = elem;
		        } else {
		            	tmpList = tmpList->left;
		            	if (!tmpList) tmpNode->left = elem;
		        }
	    	} while (tmpList);
	} else {
		list = elem;
	}  
	return 0;
}

/* Recursively clear the quarantine list 
 * Keep the memory clean
 */
void _clear_qr_list(QrSearchTree list)
{
	
	if (!list) return;
	_clear_qr_list(list->left);
	_clear_qr_list(list->right);

	free(list);
	list = NULL;
}

/* Load quarantine with content of QR_DB  
 */
void load_qr(QrSearchTree list)
{
	int fd;
	QrData tmp;
	_clear_qr_list(list);

	if ((fd = open(QR_DB, O_RDONLY, S_IRUSR)) < 0) {
		perror("Unable to open QR_DB");
		return;
	}

	while (read(fd, &tmp, sizeof(struct qr_file)) != 0) {
		if (add_to_qr_list(list, tmp)) {
			_clear_qr_list(list);
			perror("QR: Unable to load qr_list");
			return;
		}
	}

	close(fd);
}

/*
 * Search for min node in right branch
 */
QrPosition _find_min(QrSearchTree node)
{
	if (node == NULL)
		return node;

	if (node->left) 
		return _find_min(node->left);
	else
		return node;
}

/* Remove file from the qr_list 
 * Return 0 on success, -1 on error 
 */
int _rm_from_qr_list(QrSearchTree list, QrPosition node_rm)
{
	/* TODO */
	NOT_YET_IMP;
	return -1;
}

/* Search for a file in the list on filename base 
 * Return a struct node if found, NULL in other cases
 */
QrPosition search_in_qr(QrSearchTree list, const char *filename)
{
	char *base = QR_STOCK;
	QrSearchTree root = list;
	struct stat tmp;
	int exists = 1;
	if (stat(strncat(base, filename, strlen(filename)), &tmp) < 0) {
		perror("QR: Unable to find the specified file");
		exists = 0;
	}

	ino_t inum = tmp.st_ino;

	while (list) {

		ino_t cur_ino = list->data.o_ino.st_ino;

		if (inum == cur_ino) {
			if (exists) {
				break;
			} else {
				if (_rm_from_qr_list(root, list)) {
					perror("QR: Unable to remove file from qr_list");
					return NULL;
				}
			}	
		} else if (inum > cur_ino) {
			list = list->right;
		} else {
			list = list->left;
		} 
	}
	return list;
	
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
int save_qr_list(QrSearchTree list)
{
	int fd;

	if (unlink(QR_DB)) {
		perror("Unable to remove old");
		return -1;
	}

	if ((fd = open(QR_DB, O_WRONLY, S_IRUSR)) < 0) {
		perror("Unable to open QR_DB");
		return -1;
	}

	_write_node(list, fd);
	_clear_qr_list(list);

	close(fd);
	return 0;	
}

/* Move file to STOCK_QR */
void add_file_to_qr(QrSearchTree list, const char *filepath)
{
	QrData new_f;
	struct stat new_s;
	char *new_path = malloc(strlen(QR_STOCK) + 1);
	char *fn = _get_filename(filepath);

	if ((new_path == NULL )) {
		perror("QR: Couldn't allocate memory");
		return;
	}

	if (access(fn, F_OK) != -1) {
		char *tmp = malloc(strlen(fn)+(sizeof(char)*4));
		int i = 1;
		do {
			if (snprintf(tmp, strlen(fn) + sizeof(i) +1, "%s%d", fn, i) < 0) {
				perror("QR: Unable to increment filename");
				return;
			}
			i++;
		} while(access(tmp, F_OK) != -1);
		if (!strncpy(fn, tmp, strlen(tmp) + 1)) {
			perror("QR: Unable to modify filename");
			return;
		}
	}

	if (!strncpy(new_path, QR_STOCK, strlen(QR_STOCK)+1)) {
		perror("QR: Unable to create new_path");
		return;
	}

	if (!strncat(new_path, fn, strlen(fn))) {
		perror("QR: Unable to concatenate fn in new_path");
		return;
	}

	if (stat(new_path, &new_s) < 0) {
		perror("QR: Unable to get stat of file");
		return;
	}

	new_f.o_ino = new_s;
	if (!strncpy(new_f.o_path, filepath, strlen(filepath)+1)) {
		perror("QR: Unable to put current path to old path");
		return;
	}
	if (!strncpy(new_f.f_name, fn, strlen(fn)+1)) {
		perror("QR: Unable to put new filename to qr_file struct");
		return;
	}

	if (new_f.f_name == NULL)
		return;

	new_f.d_begin   = time(NULL);
	/* This will be changed when config implemented 
	 * Choice will be between expire configured and not configured
	 */
	new_f.d_expire  = 0;

        if (rename(filepath, new_path)) {
        	perror("Adding aborted: Unable to move the file");
        	return;
        }

        if (add_to_qr_list(list, new_f)) {

        }
        save_qr_list(list);
}

/* Delete definitively a file from the quarantine */
int rm_file_from_qr(QrSearchTree list, const char *filename)
{
	char *p_rm = QR_STOCK;
	if (strncat(p_rm, filename, strlen(filename)) == NULL) {
		perror("QR: Unable to create path for file to remove");
		return -1;
	}

	if (_rm_from_qr_list(list, search_in_qr(list, filename))) {
		perror("QR: Unable to remove file from qr_list");
		return -1;
	}

	if (unlink(p_rm)) {
		perror("QR: Unable to remove file from stock");
		fprintf(stderr, "WARNING: [QR] File has been removed from qr_list!\n");
		return -1;
	}
	return 0;
}

/* Restore file to its anterior state and place */
int restore_file(QrSearchTree list, const char *filename)
{
	QrPosition res_file = search_in_qr(list, filename);
	char *p_rm = QR_STOCK;
	if (res_file == NULL) {
		perror("QR ERROR: Unable to find file to restore");
		return -1;
	}
	if (_rm_from_qr_list(list, res_file)) {
		perror("QR: Unable to remove file from QR list");
		return -1;
	}
	if (strncat(p_rm, filename, strlen(filename)) == NULL) {
		perror("QR: Unable to create path for file to remove");
		return -1;
	}
	if (rename(p_rm, res_file->data.o_path)) {
		perror("Restore aborted: Unable to move the file");
		return -1;
	}
	return 0;
}
