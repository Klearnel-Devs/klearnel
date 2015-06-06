 /**
 * \file network.h 
 * \brief Network header
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 * 
 * Contains all prototypes and structures used 
 * by the network feature
 * 
 * 
 */

#define TOKEN_DB BASE_DIR "/klearnel.tk" 

#define CONF_LIST 20
#define CONF_MOD 21

typedef struct pairList
{
	char sec_token[255];
	struct pairList *next;
	struct pairList *prev;
} TPairList;

/* Execute the action received through the network socket
 * Return 0 on success and -1 on error
 */
int execute_action(const char *buf, const int c_len, const int action, const int s_cl);

void networker();

int generate_token();