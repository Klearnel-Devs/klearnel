/*-------------------------------------------------------------------------*/
/**
   \file	scan_ui.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Scanner UI file

   Contains all routines to execute actions linked to Scanner
*/
/*--------------------------------------------------------------------------*/
#include <global.h>
#include <core/ui.h>
#include <core/scanner.h>

/*-------------------------------------------------------------------------*/
/**
  \brief	The scanner form to display in command line
  \param 	path 	The path of the folder/file to add to the Scanner
  \return	TWatchElement

  
 */
/*--------------------------------------------------------------------------*/
TWatchElement _new_elem_form(char *path)
{
	TWatchElement new_elem;
	strcpy(new_elem.path,  VOID_LIST);
	new_elem.back_limit_size = -1;
	new_elem.del_limit_size = -1;
	new_elem.max_age = -1;
	int res = -1;
	int isDir = -1;
	struct stat s;
	if (stat(path, &s) < 0) {
		perror("SCAN-UI: Unable to find the specified file/folder");
		return new_elem;
	}
	strcpy(new_elem.path, path);
	if (s.st_mode & S_IFDIR) isDir = 1;
	else isDir = 0;
	if (isDir) {
		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nScan for broken symlink ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == (int)'Y') 
			new_elem.options[SCAN_BR_S] = '1';
		else new_elem.options[SCAN_BR_S] = '0';
		res = -1;

		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nScan for duplicate symlink ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_DUP_S] = '1';
		else new_elem.options[SCAN_DUP_S] = '0';
		res = -1;

		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nDelete duplicate files ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_DUP_F] = '1';
		else new_elem.options[SCAN_DUP_F] = '0';
		res = -1;

		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nScan and fix file integrity ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_INTEGRITY] = '1';
		else new_elem.options[SCAN_INTEGRITY] = '0';
		res = -1;

		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nIs this folder containing only temp files ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == 'Y') 
			new_elem.isTemp = true;
		else new_elem.isTemp = false;
		res = -1;

		if (new_elem.isTemp) {
			while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
				printf("\nClean this temp folder ? (Y/N) : ");
				if ((res = getchar()) == '\n') res = getchar();
			}
			if (toupper(res) == 'Y') 
				new_elem.options[SCAN_CL_TEMP] = '1';
			else new_elem.options[SCAN_CL_TEMP] = '0';
			res = -1;					
		} else {
			new_elem.options[SCAN_CL_TEMP] = '0';
		}

	} else {
		new_elem.options[SCAN_BR_S] 		= '0';
		new_elem.options[SCAN_DUP_S] 		= '0';
		new_elem.options[SCAN_DUP_F] 		= '0';
		new_elem.options[SCAN_INTEGRITY] 	= '0';
		new_elem.options[SCAN_CL_TEMP]		= '0';
		new_elem.isTemp 			= false;
	}
	
	while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
		printf("\nBackup large file ? (Y/N) : ");
		if ((res = getchar()) == '\n') res = getchar();
	}
	if (toupper(res) == 'Y') {
		new_elem.options[SCAN_BACKUP] = '1';
		while (new_elem.back_limit_size <= 0) {
			printf("\nEnter the limit size of file (in MB): ");
			scanf("%lf", &new_elem.back_limit_size);
			fflush(stdin);
		}
	} else {
		new_elem.options[SCAN_BACKUP] = '0';
	} 
	res = -1;

	while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
		printf("\nDelete large file ? (Y/N) : ");
		if ((res = getchar()) == '\n') res = getchar();
	}
	if (toupper(res) == 'Y') {
		new_elem.options[SCAN_DEL_F_SIZE] = '1';
		while (new_elem.del_limit_size <= 0) {
			printf("\nEnter the limit size of file (in MB): ");
			scanf("%lf", &new_elem.del_limit_size);
			fflush(stdin);
		}
	} else {
		new_elem.options[SCAN_DEL_F_SIZE] = '0';
	}
	res = -1;

	while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
		printf("\nDelete old files ? (Y/N) : ");
		if ((res = getchar()) == '\n') res = getchar();
	}
	if (toupper(res) == 'Y') {
		new_elem.options[SCAN_DEL_F_OLD] = '1';
		new_elem.options[SCAN_BACKUP_OLD] = '0';
	} else {
		new_elem.options[SCAN_DEL_F_OLD] = '0';
	} 
	res = -1;

	if (new_elem.options[SCAN_DEL_F_OLD] == '0') {
		while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
			printf("\nBackup old files ? (Y/N) : ");
			if ((res = getchar()) == '\n') res = getchar();
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_BACKUP_OLD] = '1';
		else new_elem.options[SCAN_BACKUP_OLD] = '0';
		res = -1;
	}

	if ((new_elem.options[SCAN_DEL_F_OLD] == '1') || (new_elem.options[SCAN_BACKUP_OLD] == '1')) {
		while (new_elem.max_age <= 0) {
			printf("\nEnter the limit age of files (in days): ");
			scanf("%d", &new_elem.max_age);
			fflush(stdin);
		}				
	}
	new_elem.options[SCAN_OPT_END] = '\0'; 
	return new_elem;
}

int scan_query(char **commands, int action)
{
	int len, s_cl, fd;
	int c_len = 0;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;
	TWatchElement new_elem;
	if (action == SCAN_ADD) new_elem = _new_elem_form(commands[2]);
	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("[UI] Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, SCAN_SOCK, sizeof(remote.sun_path));
	len = sizeof(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		if (action == KL_EXIT) {
			printf("Scanner service is not running\n");
			return -2;
		}
		perror("[UI] Unable to connect the scan_sock");
		return -1;
	}
	if (setsockopt(s_cl, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,	sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for reception operations");

	if (setsockopt(s_cl, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		perror("[UI] Unable to set timeout for sending operations");

	len = 20;
	res = malloc(2);
	query = malloc(len);
	if ((query == NULL) || (res == NULL)) {
		perror("[UI] Unable to allocate memory");
		goto error;
	}
	switch (action) {
		case SCAN_ADD: ;
			if (!strncmp(new_elem.path, VOID_LIST, strlen(VOID_LIST))) {
				sprintf(query, "%s", VOID_LIST);
				if (write(s_cl, query, strlen(query)) < 0) {
					perror("SCAN-UI: Unable to send file location state");
					goto error;
				}
				if (read(s_cl, res, 1) < 0) {
					perror("SCAN-UI: Unable to get location result");
				}
				goto error;
			}
			time_t timestamp = time(NULL);
			char *tmp_filename = malloc(sizeof(char)*(sizeof(timestamp)+5+strlen(SCAN_TMP)));
			if (!tmp_filename) {
				perror("SCAN-UI: Unable to allocate memory");
				goto error;
			}
			if (sprintf(tmp_filename, "%s/%d", SCAN_TMP, (int)timestamp) < 0) {
				perror("SCAN-UI: Unable to create the filename for temp scan file");
				free(tmp_filename);
				goto error;
			}
			fd = open(tmp_filename, O_WRONLY | O_TRUNC | O_CREAT);
			if (fd < 0) {
				fprintf(stderr, "SCAN-UI: Unable to create %s", tmp_filename);
				free(tmp_filename);
				goto error;
			}

			if (write(fd, &new_elem, sizeof(struct watchElement)) < 0) {
				fprintf(stderr, "SCAN-UI: Unable to write the new item to %s", tmp_filename);
				free(tmp_filename);
				goto error;
			}
			close(fd);

			c_len = strlen(tmp_filename) + 1;
			snprintf(query, len, "%d:%d", action, c_len);
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;
			}
			
			if (write(s_cl, tmp_filename, c_len) < 0) {
				perror("[UI] Unable to send args of the query");
				free(tmp_filename);
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				free(tmp_filename);
				goto error;
			}
			if (!strcmp(res, SOCK_ACK)) {
				printf("%s has been successfully added to Scanner\n", commands[2]);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				printf("An error occured while adding %s to Scanner\n", commands[2]);
			}
			free(tmp_filename);
			break;
		case SCAN_RM: ;
			c_len = strlen(commands[2]) + 1;
			snprintf(query, len, "%d:%d", action, c_len);
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				goto error;
			}
			
			if (write(s_cl, commands[2], c_len) < 0) {
				perror("[UI] Unable to send args of the query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				goto error;					
			}
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				goto error;
			}
			if (!strcmp(res, SOCK_ACK)) {
				printf("%s has been successfully removed from Scanner\n", commands[2]);
			} else if (!strcmp(res, SOCK_ABORTED)) {
				printf("An error occured while removing %s to Scanner\n", commands[2]);
			}
			break;
		case SCAN_LIST:
			snprintf(query, len, "%d:0", action);
			clock_t begin, end;
			double spent;
			begin = clock();			
			char *list_path = malloc(PATH_MAX);
			TWatchElementList *scan_list = NULL;
			if (!list_path) {
				perror("[UI] Unable to allocate memory");
				goto error;
			}
			if (write(s_cl, query, len) < 0) {
				perror("[UI] Unable to send query");
				free(list_path);
				goto error;
			} 
			if (read(s_cl, res, 1) < 0) {
				perror("[UI] Unable to get query result");
				goto error;
			}
			if (read(s_cl, list_path, PATH_MAX) < 0) {
				perror("[UI] Unable to get query result");
				free(list_path);
				goto error;	
			} 
			if (strcmp(list_path, SOCK_ABORTED) == 0) {
				perror("[UI] Action get-scan-list couldn't be executed");
				free(list_path);
				goto error;
			} 

			if (!strcmp(list_path, VOID_LIST)) {
				printf("The scanner watch list is empty!\n");
				break;
			}

			fd = open(list_path, O_RDONLY, S_IRUSR);
			if (fd < 0) {
				perror("[UI] Unable to open scan list file");
				free(list_path);
				goto error;
			}
			load_tmp_scan(&scan_list, fd);
			close(fd);
			if (unlink(list_path))
				printf("Unable to remove temporary scan list file: %s", list_path);
			print_scan(&scan_list);
			end = clock();
			spent = (double)(end - begin) / CLOCKS_PER_SEC;
			printf("\nQuery executed in: %.2lf seconds\n", spent);
			break;
		case KL_EXIT:
			snprintf(query, len, "%d:0", action);
			if (write(s_cl, query, len) < 0) {
				perror("SCAN-UI: Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("SCAN-UI: Unable to get query result");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("SCAN-UI: Unable to get query result");
				goto error;
			}
			break;
		case SCAN_FORCE:
			snprintf(query, len, "%d:0", action);
			if (write(s_cl, query, len) < 0) {
				perror("SCAN-UI: Unable to send query");
				goto error;
			}
			if (read(s_cl, res, 1) < 0) {
				perror("SCAN-UI: Unable to get query result");
				goto error;
			}
			printf("Scan task has been started\n"
				"See logs for detailed information\n");
			break;			
		default:
			printf("SCAN-UI: Unknow action. Nothing to do.\n");
	}
	free(query);
	free(res);
	shutdown(s_cl, SHUT_RDWR);
	close(s_cl);
	return 0;
error:
	free(query);
	free(res);
	shutdown(s_cl, SHUT_RDWR);
	close(s_cl);
	return -1;
}