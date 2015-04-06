 /*
 * Contains all functions required to maintain 
 configuration profiles
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <config/config.h>
#include <logging/logging.h>

void init_config()
{
	if (access(PROFILES, F_OK) == -1) {
		if (mkdir(PROFILES, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the stock", PROFILES);
			return;
		}
	}
	if (access(CFG_TMP, F_OK) == -1) {
		if (creat(CFG_TMP, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the database file", CFG_TMP);
			return;
		}
	}
	write_to_log(INFO, "%s", "Configuration Initialized without error");
}

// static int _get_data(const int sock, int *action, char **buf)
// {
// 	return 0;
// }

// int _call_related_action(const int action, char *buf, const int s_cl) 
// {
// 	return 0;
// }

// void _get_instructions()
// {
// 	return;
// }

void cfg_worker()
{
	return;
}