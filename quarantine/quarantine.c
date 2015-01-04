/*
 * Files that require user actions are stored in quarantine
 * It is the last place before physical suppression of any file
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <quarantine/quarantine.h>

/* Init a node of the quarantine with all attributes at NULL */
static struct qr_node* new_qr_node(){
	struct qr_node *tmp = malloc(sizeof(struct qr_node));
	if(tmp == NULL){
		perror("Couldn't allocate memory");
		goto out;
	}
	tmp->data = NULL;
	tmp->left = NULL;
	tmp->right = NULL;
out:
	return tmp;
}

/* Attach the shared memory of quarantine tree to list 
 * Return a table with the sem_id and the shm_id created
 */
static int* at_qr_shm(struct qr_node **list, int shmflags){
	int *qr_ipc = malloc(2*sizeof(int));
	if(qr_ipc == NULL){
		perror("Unable to allocate memory");
		goto out;
	}
	key_t qr_sem_key = ftok(IPC_RAND, QR_SEM);
	qr_ipc[0] = semget(qr_sem_key, 1, IPC_CREAT | IPC_PERMS);

	key_t qr_shm_key = ftok(IPC_RAND, QR_SHM);
	qr_ipc[1] = shmget(qr_shm_key, QR_SHM_SIZE, shmflags);

	*list = (struct qr_node *) shmat(qr_ipc[1], NULL, 0);
out:
	return qr_ipc;
}

/* Add a file to the quarantine list */
void add_to_qr_list(struct qr_node** list, struct qr_file* new){
	struct qr_node *tmpNode;
	struct qr_node *tmpList = *list;
	struct qr_node *elem;

	if((elem = new_qr_node()) == NULL){
		perror("Unable to create new qr node");
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
void clear_qr_list(struct qr_node** list){
	struct qr_node *tmp = *list;
	
	if(!list) return;
	if(tmp->left)  clear_qr_list(&tmp->left);
	if(tmp->right) clear_qr_list(&tmp->right);

	free(tmp);
	*list = NULL;
}

/* Load quarantine with content of QR_DB 
 * This function initialize the QR_SHM 
 */
void load_qr(){
	int fd;
	struct qr_file *tmp;
	struct qr_node *list = NULL;

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
		add_to_qr_list(&list, tmp);

	close(fd);
	
	struct qr_node *sh_list;
	int *qr_ipc = at_qr_shm(&sh_list, S_IWUSR);

	wait_crit_area(qr_ipc[0], 0);
	sem_down(qr_ipc[0], 0);
	sh_list = list;
	sem_up(qr_ipc[0], 0);

	shmdt(&qr_ipc[1]);
}

/* Search for a file in the list on inode number base */
static struct qr_node* search_in_qr(struct qr_node *list, const ino_t inum){
	struct qr_node *tmpList = list;
	struct qr_node *tmpNode;
        
	tmpNode = new_qr_node();

	while(tmpList){

		ino_t cur_ino = tmpList->data->o_ino.st_ino;

		if(inum == cur_ino) break;
		else if(inum > cur_ino) tmpList = tmpList->right;
		else tmpList = tmpList->left; 
	}

	tmpNode = tmpList;

	return tmpNode;
}

/* Get filename in a path */
static char* get_filename(const char *path){
	char sep = '/';
	char *name;
	int i, l_occ, lg_tot, lg_sub;

  	lg_tot = strlen(path);
	for(i = 0; i < lg_tot; i++) 
		if(path[i] == sep) 
			l_occ = i;

	if((name = malloc(lg_tot - i) + 1) == NULL){
		perror("Couldn't allocate memory");
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
int save_qr_list(){
	int fd;
	int *qr_ipc;
	struct qr_node *list;

	if(unlink(QR_DB)){
		perror("Unable to remove old");
		return -1;
	}

	if((fd = open(QR_DB, O_WRONLY, S_IRUSR)) < 0){
		perror("Unable to open QR_DB");
		return -1;
	}

	qr_ipc = at_qr_shm(&list, S_IRUSR | S_IRUSR);

	wait_crit_area(qr_ipc[0], 0);
	sem_down(qr_ipc[0], 0);

	write_node(&list, fd);
	clear_qr_list(&list);

	sem_up(qr_ipc[0], 0);

	close(fd);
	shmdt(&qr_ipc[1]);
	return 0;	
}

/* Move file to STOCK_QR */
void add_file_to_qr(const char* file){
	struct qr_node *list;
	struct qr_file *new_f = malloc(sizeof(struct qr_file));
	struct stat new_s;
	char *new_path = malloc(strlen(QR_STOCK) + 1);
	char *fn = get_filename(file);

	if((new_f == NULL) || (new_path == NULL )){
		perror("Couldn't allocate memory");
		return;
	}

	int *qr_ipc = at_qr_shm(&list, S_IWUSR | S_IRUSR);

	strcpy(new_path, QR_STOCK);
	strcat(new_path, fn);

	stat(new_path, &new_s);

	new_f->o_ino = new_s;
	strcpy(new_f->o_path, file);
	strcpy(new_f->f_name, fn);

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

        wait_crit_area(qr_ipc[0], 0);
	sem_down(qr_ipc[0], 0);
	add_to_qr_list(&list, new_f);
	sem_up(qr_ipc[0], 0);

        shmdt(&qr_ipc[1]);
}

/* Delete definitively a file from the quarantine */
void rm_file_from_qr(const char* file){
	not_yet_implemented(__func__);
}

/* Restore file to its anterior state and place */
void restore_file(const char* file){
	not_yet_implemented(__func__);
}