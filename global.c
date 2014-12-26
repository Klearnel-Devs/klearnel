/*
 * Contains all functions used by the whole module
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>

/* To use when you are going to work in shared memo */
void sem_down(int sem_id, int sem_channel){
	struct sembuf op;
	op.sem_num 	= sem_channel;
	op.sem_op 	= -1;
	op.sem_flg 	=  0;
	semop(sem_id, &op, 1);
} 

/* To use when you finish your work in shared memo */
void sem_up(int sem_id, int sem_channel){
	struct sembuf op;
	op.sem_num 	= sem_channel;
	op.sem_op 	= 1;
	op.sem_flg 	= IPC_NOWAIT;
	semop(sem_id, &op, 1);
}

/* Reset the sema to 1 */
void sem_reset(int sem_id, int sem_channel){
	semctl(sem_id, sem_channel, SETVAL, 1);
}

/* Check if shared mem is readable */
bool isShMemReadable(int sem_id, int sem_channel){
	if(semctl(sem_id, sem_channel, GETVAL) == 1) return true;
	return false;
}

/* Return sem_channel of sem_id value */
int sem_val(int sem_id, int sem_channel){
	return semctl(sem_id, sem_channel, GETVAL);
}

/* Set sem_channel of sem_id at val */
void sem_put(int sem_id, int sem_channel, int _val){
	semctl(sem_id, sem_channel, SETVAL, _val);
}