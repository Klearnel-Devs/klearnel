 /**
 * \file networker.c
 * \brief Networker File
 * \author Copyright (C) 2014, 2015 Klearnel-Devs 
 * 
 * This file contains all functions that are needed
 * to communicate with klearnel-manager
 * through the network socket
 *
 */
#include <global.h>
#include <net/network.h>

int execute_action(const char *buf, const int action, const int s_cl)
{
	switch (action) {
		case NET_TK:
			NOT_YET_IMP;
			break;
		case NET_ROOT:
			NOT_YET_IMP;
			break;
		default:
			LOG(WARNING, "Unknow command received");
	}
	
	return 0;
}