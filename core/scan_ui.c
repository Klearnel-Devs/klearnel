/*
 * Contains all routines to execute actions linked to Scanner
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <core/ui.h>
#include <logging/logging.h>
#include <core/scanner.h>

TWatchElement _new_elem_form()
{
	TWatchElement new_elem;
	strcpy(new_elem.path, EMPTY_PATH);
	new_elem.back_limit_size = -1;
	new_elem.del_limit_size = -1;
	new_elem.max_age = -1;
	char res = ' ';
	int isDir = -1;
	struct stat s;
	printf("Enter the path to folder/file to scan: ");
	if (!fgets(new_elem.path, PATH_MAX, stdin)) {
		perror("UI: Unable to read the path");
		return new_elem;
	}
	fflush(stdin);

	if (stat(new_elem.path, &s) < 0) {
		perror("SCAN-UI: Unable to find the specified file/folder");
		return new_elem;
	}
	if (s.st_mode & S_IFDIR) isDir = 1;
	else isDir = 0;
	if (isDir) {
		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nScan for broken symlink ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_BR_S] = '1';
		else new_elem.options[SCAN_BR_S] = '0';
		res = ' ';

		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nScan for duplicate symlink ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_DUP_S] = '1';
		else new_elem.options[SCAN_DUP_S] = '0';
		res = ' ';

		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nDelete duplicate files ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_DUP_F] = '1';
		else new_elem.options[SCAN_DUP_F] = '0';
		res = ' ';

		if (!new_elem.options[SCAN_DUP_F]) {
			while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
				printf("\nFuse duplicate files ? (Y/N) : ");
				res = getchar();
				fflush(stdin);
			}
			if (toupper(res) == 'Y') 
				new_elem.options[SCAN_FUSE] = '1';
			else new_elem.options[SCAN_FUSE] = '0';
			res = ' ';
		} else {
			new_elem.options[SCAN_FUSE] = '0';
		}

		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nScan and fix file integrity ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_INTEGRITY] = '1';
		else new_elem.options[SCAN_INTEGRITY] = '0';
		res = ' ';

		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nIs this folder containing only temp files ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.isTemp = true;
		else new_elem.isTemp = false;
		res = ' ';

		if (new_elem.isTemp) {
			while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
				printf("\nClean this temp folder ? (Y/N) : ");
				res = getchar();
				fflush(stdin);
			}
			if (toupper(res) == 'Y') 
				new_elem.options[SCAN_CL_TEMP] = '1';
			else new_elem.options[SCAN_CL_TEMP] = '0';
			res = ' ';					
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
	while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
		printf("\nBackup large file ? (Y/N) : ");
		res = getchar();
		fflush(stdin);
	}
	if (toupper(res) == 'Y') {
		new_elem.options[SCAN_BACKUP] = '1';
		char size[1024];
		while (new_elem.back_limit_size <= 0) {
			printf("\nEnter the limit size of file (in MB): ");
			if (!fgets(size, 1024, stdin)) {
				perror("SCAN-UI: Unable to read limit size");
				return new_elem;
			}
			sscanf(size, "%lf", &new_elem.back_limit_size);
			fflush(stdin);
		}
	} else {
		new_elem.options[SCAN_BACKUP] = '0';
	} 
	res = ' ';

	while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
		printf("\nDelete large file ? (Y/N) : ");
		res = getchar();
		fflush(stdin);
	}
	if (toupper(res) == 'Y') {
		new_elem.options[SCAN_DEL_F_SIZE] = '1';
		char size[1024];
		while (new_elem.del_limit_size <= 0) {
			printf("\nEnter the limit size of file (in MB): ");
			if (!fgets(size, 1024, stdin)) {
				perror("SCAN-UI: Unable to read limit size");
				return new_elem;
			}
			sscanf(size, "%lf", &new_elem.del_limit_size);
			fflush(stdin);
		}
	} else {
		new_elem.options[SCAN_DEL_F_SIZE] = '0';
	}
	res = ' ';

	while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
		printf("\nDelete old files ? (Y/N) : ");
		res = getchar();
		fflush(stdin);
	}
	if (toupper(res) == 'Y') 
		new_elem.options[SCAN_DEL_F_OLD] = '1';
	else new_elem.options[SCAN_DEL_F_OLD] = '0';
	res = ' ';

	if (!new_elem.options[SCAN_DEL_F_OLD]) {
		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nBackup old files ? (Y/N) : ");
			res = getchar();
			fflush(stdin);
		}
		if (toupper(res) == 'Y') 
			new_elem.options[SCAN_BACKUP_OLD] = '1';
		else new_elem.options[SCAN_BACKUP_OLD] = '0';
		res = ' ';
	}

	if (new_elem.options[SCAN_DEL_F_OLD] || new_elem.options[SCAN_BACKUP_OLD]) {
		char age[100];
		while ((toupper(res) != 'Y') || (toupper(res) != 'N')) {
			printf("\nEnter the limit age of files (in days): ");
			if (!fgets(age, 100, stdin)) {
				perror("SCAN-UI: Unable to read limit age");
				return new_elem;
			}
			sscanf(age, "%f", &new_elem.max_age);
			fflush(stdin);
		}				
	}
	return new_elem;
}

int scan_query(int nb, char **commands, int action)
{
	int len, s_cl;
	char *query, *res;;
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
			TWatchElement new_elem = _new_elem_form();

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
			printf("SCAN-UI: Unknow action. Nothing to do\n");
	}
	return 0;
}