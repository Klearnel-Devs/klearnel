/*
 * Contains all functions used by the whole module
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>


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
