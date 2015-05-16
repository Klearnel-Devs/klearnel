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

typedef struct pair
{
	int id;
	int sec_token;
} TPair;

/* Execute the action received through the network socket
 * Return 0 on success and -1 on error
 */
int execute_action(const char *buf, const int action, const int s_cl);
int sendResult();

void networker();
