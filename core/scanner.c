/*
 * Scanner manage the automated process for folders / files it had to follow
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>
#include <core/scanner.h>

static TWatchElementList elem_list = NULL;


int init_scanner()
{
	if (access(SCAN_DB, F_OK) == -1) {
		if (creat(SCAN_DB, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s:%d: %s", __func__, __LINE__, "Unable to create the scanner database");
			return -1;
		}
	}
	return 0;
}

int load_watch_list()
{
	int fd = 0;

	if ((fd = open(SCAN_DB, O_RONLY)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	return 0;
}