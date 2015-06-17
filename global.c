/*-------------------------------------------------------------------------*/
/**
   \file	global.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Global file

   Contains all functions used by the whole module
*/
/*--------------------------------------------------------------------------*/
#include <global.h>
#include <logging/logging.h>

int get_data(const int sock, int *action, char **buf, int c_len)
{
	char *a_type = malloc(c_len);
	int len;
	if (a_type == NULL) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
		return -1;
	}

	if (read(sock, a_type, c_len) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Error while receiving data through socket");
		return -1;
	}

	if (SOCK_ANS(sock, SOCK_ACK) < 0) {
		write_to_log(WARNING, "%s - %d - %s", __func__, __LINE__, "Unable to send ack in socket");
		free(a_type);
		return -1;
	}


	if (strncmp(a_type, VOID_LIST, strlen(VOID_LIST)) == 0) {
		return -1; // Stop here if there was no information read from socket
	}

	*action = atoi(strtok(a_type, ":"));
	len = atoi(strtok(NULL, ":"));

	if (len > 0) {
		*buf = malloc(sizeof(char)*len);
		if (*buf == NULL) {
			if (SOCK_ANS(sock, SOCK_RETRY) < 0)
				write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send retry");
			write_to_log(FATAL,"%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
			free(a_type);
			return -1;
		}

		if (read(sock, *buf, len) < 0) {
			if (SOCK_ANS(sock, SOCK_NACK) < 0)
				write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send nack");
			write_to_log(FATAL,"%s - %d - %s", __func__, __LINE__, "Unable to read data from socket");
			free(a_type);
			return -1;			
		}

		if(SOCK_ANS(sock, SOCK_ACK) < 0) {
			write_to_log(WARNING,"%s - %d - %s", __func__, __LINE__, "Unable to send ack");
			free(a_type);
			return -1;
		}
	}
	free(a_type);
	return len;
}

void sem_down(int sem_id, int sem_channel){
	struct sembuf op;
	op.sem_num 	= sem_channel;
	op.sem_op 	= -1;
	op.sem_flg 	=  0;
	semop(sem_id, &op, 1);
} 

void sem_up(int sem_id, int sem_channel){
	struct sembuf op;
	op.sem_num 	= sem_channel;
	op.sem_op 	= 1;
	op.sem_flg 	= IPC_NOWAIT;
	semop(sem_id, &op, 1);
}

void sem_reset(int sem_id, int sem_channel){
	semctl(sem_id, sem_channel, SETVAL, 1);
}

int is_crit_area_free(int sem_id, int sem_channel){
	if(semctl(sem_id, sem_channel, GETVAL) == 1) return 1;
	return 0;
}

void wait_crit_area(int sem_id, int sem_channel){
	while(!is_crit_area_free(sem_id, sem_channel)) usleep(2000);
}

int sem_val(int sem_id, int sem_channel){
	return semctl(sem_id, sem_channel, GETVAL);
}

void sem_put(int sem_id, int sem_channel, int val){
	semctl(sem_id, sem_channel, SETVAL, val);
}

void not_yet_implemented(const char* func){
	printf("%s: Not yet implemented\n", func);
}
