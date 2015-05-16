/*-------------------------------------------------------------------------*/
/**
   \file	scanner.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Scanner module file

   Scanner manage the automated process for folders / files it had to follow
*/
/*--------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>
#include <core/scanner.h>

static TWatchElementList* watch_list = NULL;

/*-------------------------------------------------------------------------*/
/**
  \brief        Adds TWatchElement to temporary TWatchElemtnList
  \param        elem 	The TWatchElement to add
  \param        list 	The temporary TWatchElementList
  \return       Return 0 on success and -1 on error	

  
 */
/*--------------------------------------------------------------------------*/
int _add_tmp_watch_elem(TWatchElement elem, TWatchElementList **list) 
{
	TWatchElementNode* node = malloc(sizeof(struct watchElementNode));
	if (!node) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	node->element = elem;
	node->next = NULL;
	if (!(*list)) {
		(*list) = malloc(sizeof(struct watchElementList));
		if (!(*list)) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		(*list)->first = node;
		(*list)->last = node;
		(*list)->count = 1;
		return 0;
	}
	
	node->prev = (*list)->last;
	(*list)->last->next = node;
	(*list)->last = node;
	(*list)->count++;
	return 0;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Checks for broken and duplicate symlinks
  \param        data 	The element to verify
  \param 	action	The action to take
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _checkSymlinks(TWatchElement data, int action) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Backup of files and folders larger than X size
  \param        data 	The element to verify
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _backupFiles(TWatchElement data) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Delete files and folders larger than X size
  \param        data 	The element to verify
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _deleteFiles(TWatchElement data) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Deletes or Fuses duplicate files in a folder
  \param        data 	The element to verify
  \param 	action	The action to take
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _handleDuplicates(TWatchElement data, int action) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Checks and repairs incoherent permissions
  \param        data 	The element to verify
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _checkPermissions(TWatchElement data) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Cleans folder at a specified time
  \param        data 	The element to verify
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _cleanFolder(TWatchElement data) {
	NOT_YET_IMP;
}
/*-------------------------------------------------------------------------*/
/**
  \brief        Deletes or backs up files and folders older than X
  \param        data 	The element to verify
  \param 	action	The action to take
  \return       void	

  
 */
/*--------------------------------------------------------------------------*/
void _oldFiles(TWatchElement data, int action) {
	NOT_YET_IMP;
}

int perform_event() 
{
	int i;
	if (watch_list->first == NULL) {
		return 0;
	}
	SCAN_LIST_FOREACH(watch_list, first, next, cur) {
		for(i = 0; i < OPTIONS; i++) {
			switch(i) {
				case SCAN_BR_S :
				case SCAN_DUP_S : 
					if (cur->element.options[i] == '1') 
						_checkSymlinks(cur->element, i);
					break;
				case SCAN_BACKUP : 
					if (cur->element.options[i] == '1')
						_backupFiles(cur->element);
					break; 
				case SCAN_DEL_F_SIZE : 
					if (cur->element.options[i] == '1')
						_deleteFiles(cur->element); 
					break;
				case SCAN_DUP_F :
				case SCAN_FUSE : 
					if (cur->element.options[i] == '1')
						_handleDuplicates(cur->element, i);
					break;
				case SCAN_INTEGRITY : 
					if (cur->element.options[i] == '1')
						_checkPermissions(cur->element); 
					break;
				case SCAN_CL_TEMP : 
					if (cur->element.options[i] == '1')
						_cleanFolder(cur->element); 
					break;
				case SCAN_DEL_F_OLD : 
				case SCAN_BACKUP_OLD : 
					if (cur->element.options[i] == '1')
						_oldFiles(cur->element, i); 
					break;
				default: break;
			}
		}
	}
	return 0;
}

int init_scanner()
{
	if (access(SCAN_DB, F_OK) == -1) {
		if (creat(SCAN_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s:%d: %s", __func__, __LINE__, "Unable to create the scanner database");
			return -1;
		}
	}
	if (access(SCAN_TMP, F_OK) == -1) {
		if (mkdir(SCAN_TMP, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			write_to_log(FATAL, "%s:%d: %s", __func__, __LINE__, "Unable to create the SCAN_TMP folder");
			return -1;
		}
	}
	return 0;
}

int add_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* node = malloc(sizeof(struct watchElementNode));
	if (!node) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	node->element = elem;
	node->next = NULL;
	if (!watch_list) {
		watch_list = malloc(sizeof(struct watchElementList));
		if (!watch_list) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		watch_list->first = node;
		watch_list->last = node;
		watch_list->count = 1;
		return 0;
	}
	
	node->prev = watch_list->last;
	watch_list->last->next = node;
	watch_list->last = node;
	watch_list->count++;
	return 0;
}

int remove_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* item = watch_list->first;
	if (!item) return 0;
	while (item) {
		if (strcmp(item->element.path, elem.path) == 0) {
			if (strcmp(watch_list->first->element.path, item->element.path) == 0) {
				watch_list->first = item->next;
			}
			if (strcmp(watch_list->last->element.path, item->element.path) == 0) {
				watch_list->last = item->prev;
			}
			if (item->next) item->next->prev = item->prev;
			if (item->prev)	item->prev->next = item->next;
			free(item);
			watch_list->count--;
			return 0;
		}
		item = item->next;
	}
	write_to_log(FATAL, "%s:%d: Element \"%s\" to remove not found", 
		__func__, __LINE__, elem.path);
	return -1;
}

int load_watch_list()
{
	int fd = 0;
	TWatchElement tmp;
	if ((fd = open(SCAN_DB, O_RDONLY)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	while(read(fd, &tmp, sizeof(struct watchElement)) != 0) {
		if (add_watch_elem(tmp) < 0) {
			write_to_log(URGENT, "%s:%d: Unable to add \"%s\" to the watch list", 
				__func__, __LINE__, tmp.path);
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}

int get_watch_list()
{
	NOT_YET_IMP;
	return 0;
}

void clear_watch_list()
{
	if (watch_list->first != NULL) {
		SCAN_LIST_FOREACH(watch_list, first, next, cur) {
			if (cur->prev) {
			    	free(cur->prev);
			}
		}
	}
	free(watch_list);
	watch_list = NULL;
}

TWatchElement get_watch_elem(const char* path) 
{
	TWatchElement tmp;
	strcpy(tmp.path, EMPTY_PATH);
	if (!watch_list) {
		if (load_watch_list() < 0) {
			LOG(WARNING, "Unable to load the watch list");
			return tmp;			
		}
	}
	int i;
	TWatchElementNode* item = watch_list->first;
	for (i = 0; i < watch_list->count; i++) {
		if (!strcmp(item->element.path, path)) {
			return item->element;
		}
		item = item->next;
	}
	return tmp;
}

int save_watch_list(int custom)
{
	int fd;
	if (watch_list == NULL) return 0;
	if (custom >= 0) {
		fd = custom;
	} else if ((fd = open(SCAN_DB, O_WRONLY | O_TRUNC)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	SCAN_LIST_FOREACH(watch_list, first, next, cur) {
		if (write(fd, &cur->element, sizeof(struct watchElement)) < 0) {
			write_to_log(FATAL, "%s:%s: Unable to write \"%s\" in SCAN_DB", 
				__func__, __LINE__, cur->element.path);
			return -1;
		}
	}
	clear_watch_list();
	if (custom < 0) {
		close(fd);
	}
	return 0;
}

void print_scan(TWatchElementList **list)
{
	clock_t begin, end;
	double spent;
	begin = clock();
	printf("Scanner elements:\n");
	SCAN_LIST_FOREACH((*list), first, next, cur) {
		printf("\nElement \"%s\":\n", cur->element.path);
		printf("\t- Options: %s\n", cur->element.options);
	}
	end = clock();
	spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("\nQuery executed in: %.2lf seconds\n", spent);
}

void load_tmp_scan(TWatchElementList **list, int fd)
{
	TWatchElement tmp;
	while (read(fd, &tmp, sizeof(struct watchElement)) != 0) {
		if (_add_tmp_watch_elem(tmp, list) != 0){
			perror("Out of memory!");
			exit(EXIT_FAILURE);
		}
	}
}

void scanner_worker()
{
	int len, s_srv, s_cl;
	// CHECK WITH ANTOINE
	int c_len = 20;
	int task = 0;
	struct sockaddr_un server;
	int oldmask = umask(0);
	if ((s_srv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		write_to_log(WARNING, "%s:%d: %s", __func__, __LINE__, "Unable to open the socket");
		return;
	}
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, SCAN_SOCK, strlen(SCAN_SOCK) + 1);
	unlink(server.sun_path);
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if(bind(s_srv, (struct sockaddr *)&server, len) < 0) {
		write_to_log(WARNING, "%s:%d: %s", __func__, __LINE__, "Unable to bind the socket");
		return;
	}
	umask(oldmask);
	listen(s_srv, 10);

	do {
		struct timeval to_socket;
		to_socket.tv_sec 	= SOCK_TO;
		to_socket.tv_usec = 0;
		struct timeval to_select;
		to_select.tv_sec 	= SEL_TO;
		to_select.tv_usec = 0;
		fd_set fds;
		int res;

		FD_ZERO (&fds);
		FD_SET (s_srv, &fds);

		struct sockaddr_un remote;
		char *buf = NULL;

		res = select (s_srv + 1, &fds, NULL, NULL, &to_select);
		if (res > 0) {
			if (FD_ISSET(s_srv, &fds)) {
				len = sizeof(remote);
				if ((s_cl = accept(s_srv, (struct sockaddr *)&remote, (socklen_t *)&len)) == -1) {
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to accept the connection");
					continue;
				}
				if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to set timeout for reception operations");

				if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&to_socket, sizeof(to_socket)) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to set timeout for sending operations");		

				if (get_data(s_cl, &task, &buf, c_len) < 0) {
					free(buf);
					close(s_cl);
					write_to_log(NOTIFY, "%s:%d: %s", 
						__func__, __LINE__, "_get_data FAILED");
					continue;
				}
				perform_task(task, buf, s_cl);
				free(buf);
				close(s_cl);
			}
		} else {
			perform_event();
		}
	} while (task != KL_EXIT);
	close(s_srv);
	unlink(server.sun_path);
	exit(EXIT_SUCCESS);
}

int perform_task(const int task, const char *buf, const int s_cl) 
{
	if (load_watch_list() < 0) {
		LOG(WARNING, "Unable to load the watch list");
		return -1;
	}
	switch (task) {
		case SCAN_ADD:
			if (buf == NULL) {
				LOG(URGENT, "Buffer received is empty");
				return -1;
			}
			int fd = open(buf, O_RDONLY);
			if (fd <= 0) {
				write_to_log(URGENT,"%s:%d: Unable to open %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			TWatchElement new;
			if (read(fd, &new, sizeof(struct watchElement)) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to read data from %s", 
					__func__, __LINE__, buf);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			close(fd);
			if (add_watch_elem(new) < 0) {
				write_to_log(URGENT,"%s:%d: Unable to add %s to watch_list", 
					__func__, __LINE__, new.path);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;
			}
			unlink(buf);
			save_watch_list(-1);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case SCAN_RM:
			if (buf == NULL) {
				LOG(URGENT, "Buffer received is empty");
				return -1;				
			}
			TWatchElement tmp = get_watch_elem(buf);
			if (!strcmp(tmp.path, EMPTY_PATH)) {
				LOG(NOTIFY, "Unable to find the element to remove");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			if (remove_watch_elem(tmp) < 0) {
				write_to_log(NOTIFY, "%s:%d: Unable to remove %s", 
					__func__, __LINE__, tmp.path);
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0)
					write_to_log(WARNING, "%s:%d: %s", 
						__func__, __LINE__, "Unable to send aborted");
				return -1;				
			}
			save_watch_list(-1);
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		case SCAN_LIST: ;
			time_t timestamp = time(NULL);
			int tmp_stock;
			char *path_to_list = malloc(sizeof(char)*(sizeof(timestamp)+20));
			char *file = malloc(sizeof(char)*(sizeof(timestamp)+30));
			if (!path_to_list || !file) {
				write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				return 0;
			}
			sprintf(path_to_list, "%s", SCAN_TMP);
			int oldmask = umask(0);
			if (access(path_to_list, F_OK) == -1) {
				if (mkdir(path_to_list, ALL_RWX)) {
					write_to_log(WARNING, "%s - %d - %s:%s", __func__, __LINE__, "Unable to create the tmp_stock folder", path_to_list);
					if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
						write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
					}
					free(path_to_list);
					return 0;
				}
			}
			sprintf(file, "%s/%d", path_to_list, (int)timestamp);
			tmp_stock = open(file, O_WRONLY | O_TRUNC | O_CREAT, ALL_RWX);
			if (tmp_stock < 0) {
				write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to open tmp_stock");
				if (SOCK_ANS(s_cl, SOCK_ABORTED) < 0) {
					write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send aborted");
				}
				free(path_to_list);
				free(file);
				return 0;
			}
			save_watch_list(tmp_stock);
			close(tmp_stock);
			umask(oldmask);
			write(s_cl, file, PATH_MAX);
			free(path_to_list);
			free(file);
			break;
		case KL_EXIT:
			LOG(INFO, "Received KL_EXIT command");
			exit_scanner();
			LOG_DEBUG;
			SOCK_ANS(s_cl, SOCK_ACK);
			break;
		default:
			LOG(NOTIFY, "Unknown task. Scan execution aborted");
	}

	return 0;
}

int exit_scanner()
{
	if (save_watch_list(-1) < 0) {
		LOG(FATAL, "Unable to save the watch_list");
		return -1;
	}
	return 0;
}
