/*
 * Contains all routines to execute actions linked to Scanner
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <core/ui.h>
#include <core/scanner.h>

TWatchElement _new_elem_form(char *path)
{
	TWatchElement new_elem;
	strcpy(new_elem.path, path);
	new_elem.back_limit_size = -1;
	new_elem.del_limit_size = -1;
	new_elem.max_age = -1;
	int res = -1;
	int isDir = -1;
	struct stat s;
	if (stat(new_elem.path, &s) < 0) {
		perror("SCAN-UI: Unable to find the specified file/folder");
		return new_elem;
	}
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

		if (new_elem.options[SCAN_DUP_F] == '0') {
			while ((toupper(res) != 'Y') && (toupper(res) != 'N')) {
				printf("\nFuse duplicate files ? (Y/N) : ");
				if ((res = getchar()) == '\n') res = getchar();
			}
			if (toupper(res) == 'Y') 
				new_elem.options[SCAN_FUSE] = '1';
			else new_elem.options[SCAN_FUSE] = '0';
			res = -1;
		} else {
			new_elem.options[SCAN_FUSE] = '0';
		}

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
		new_elem.options[SCAN_FUSE] 		= '0';
		new_elem.options[SCAN_INTEGRITY] 	= '0';
		new_elem.options[SCAN_CL_TEMP]		= '0';
		new_elem.isTemp 			= false;
	}
	/* WARNING BACKUP NEED TO BE IMPLEMENTED */
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
			scanf("%f", &new_elem.max_age);
			fflush(stdin);
		}				
	}
	new_elem.options[SCAN_OPT_END] = '\0';
	return new_elem;
}

int scan_query(int nb, char **commands, int action)
{
	int len, s_cl;
	char *query, *res;
	struct sockaddr_un remote;
	struct timeval timeout;
	timeout.tv_sec 	= SOCK_TO;
	timeout.tv_usec	= 0;

	if ((s_cl = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("[UI] Unable to create socket");
		return -1;
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, SCAN_SOCK, strlen(SCAN_SOCK) + 1);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s_cl, (struct sockaddr *)&remote, len) == -1) {
		perror("[UI] Unable to connect the qr_sock");
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
		return -1;
	}
	switch (action) {
		case SCAN_ADD: ;
			TWatchElement new_elem = _new_elem_form(commands[2]);
			time_t timestamp = time(NULL);
			char *tmp_filename = malloc(sizeof(char)*(sizeof(timestamp)+5+strlen(SCAN_TMP)));
			int fd;
			if (!tmp_filename) {
				perror("SCAN-UI: Unable to allocate memory");
				return -1;
			}
			if (sprintf(tmp_filename, "%s/%d", SCAN_TMP, (int)timestamp) < 0) {
				perror("SCAN-UI: Unable to create the filename for temp scan file");
				return -1;
			}
			fd = open(tmp_filename, O_WRONLY | O_TRUNC | O_CREAT);
			if (fd < 0) {
				fprintf(stderr, "SCAN-UI: Unable to create %s", tmp_filename);
				return -1;
			}

			if (write(fd, &new_elem, sizeof(struct watchElement)) < 0) {
				fprintf(stderr, "SCAN-UI: Unable to write the new item to %s", tmp_filename);
				return -1;
			}
			close(fd);


			break;
		case SCAN_RM:
			NOT_YET_IMP;
			break;
		case SCAN_LIST:
			NOT_YET_IMP;
			break;
		case KL_EXIT:
			NOT_YET_IMP;
			break;
		default:
			printf("SCAN-UI: Unknow action. Nothing to do.\n");
	}
	return 0;
}