/*
 * Scanner manage the automated process for folders / files it had to follow
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>
#include <core/scanner.h>

static TWatchElementList* watch_list = NULL;


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

int add_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* node = malloc(sizeof(struct watchElementNode));
	if (!node) {
		LOG(FATAL, "Unable to allocate memory");
		return -1;
	}
	node->element = elem;
	node->next = NULL;
	if (!watch_list) {
		watch_list = malloc(sizeof(struct watchElementList));
		if (!watch_list) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		watch_list->first = node;
		watch_list->last = node;
		watch_list->count = 1;
		return 0;
	}
	
	node->prev = watch_list->last;
	watch_list->last->next = node;
	watch_list->last = node;
	watch_list->count++;
	return 0;
}

int remove_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* item = watch_list->first;
	if (!item) return 0;
	while (item) {
		if (strcmp(item->element.path, elem.path) == 0) {
			if (strcmp(watch_list->first->element.path, item->element.path) == 0) {
				watch_list->first = item->next;
			}
			if (strcmp(watch_list->last->element.path, item->element.path) == 0) {
				watch_list->last = item->prev;
			}
			if (item->next) item->next->prev = item->prev;
			if (item->prev)	item->prev->next = item->next;
			free(item);
			watch_list->count--;
			return 0;
		}
	}
	write_to_log(FATAL, "%s:%d: Element \"%s\" to remove not found", 
		__func__, __LINE__, elem.path);
	return -1;
}

int load_watch_list()
{
	int fd = 0;
	TWatchElement tmp;

	if ((fd = open(SCAN_DB, O_RDONLY)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}

	while(read(fd, &tmp, sizeof(struct watchElement)) != 0) {
		if (add_watch_elem(tmp) < 0) {
			write_to_log(URGENT, "%s:%d: Unable to add \"%s\" to the watch list", 
				__func__, __LINE__, tmp.path);
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}

int save_watch_list()
{
	int fd, i;

	if ((fd = open(SCAN_DB, O_WRONLY)) < 0) {
		LOG(URGENT, "Unable to open the SCAN_DB");
		return -1;
	}
	TWatchElementNode* item = watch_list->first;
	for(i = 0; i < watch_list->count; i++) {
		if (write(fd, item->element, sizeof(struct watchElement)) < 0) {
			write_to_log(FATAL, "%s:%s: Unable to write \"%s\" in SCAN_DB", 
				__func__, __LINE__, item->element.path);
			return -1;
		}
	}
	free(watch_list);
	close(fd);
	return 0;
}
