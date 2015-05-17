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

#define NET_TK 		100
#define NET_ROOT 	101

typedef struct pair
{
	int id;
	int sec_token;
} TPair;

/* Execute the action received through the network socket
 * Return 0 on success and -1 on error
 */
int execute_action(const char *buf, const int action, const int s_cl);

void networker();

int generate_token();