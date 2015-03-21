/*
 * Scanner manage the automated process for folders / files it had to follow
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 */
#include <global.h>
#include <quarantine/quarantine.h>
#include <logging/logging.h>
#include <core/scanner.h>

static TWatchElementList* elem_list = NULL;


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
	if (!elem_list) {
		elem_list = malloc(sizeof(struct watchElementList));
		if (!elem_list) {
			LOG(FATAL, "Unable to allocate memory");
			return -1;
		}
		node->prev = NULL;
		elem_list->first = node;
		elem_list->last = node;
		elem_list->count = 1;
		return 0;
	}
	
	node->prev = elem_list->last;
	elem_list->last->next = node;
	elem_list->last = node;
	elem_list->count++;
	return 0;
}

int remove_watch_elem(TWatchElement elem) 
{
	TWatchElementNode* item = elem_list->first;
	if (!item) return 0;
	while (item) {
		if (strcmp(item->element.path, elem.path) == 0) {
			if (strcmp(elem_list->first->element.path, item->element.path) == 0) {
				elem_list->first = item->next;
			}
			if (strcmp(elem_list->last->element.path, item->element.path) == 0) {
				elem_list->last = item->prev;
			}
			if (item->next) item->next->prev = item->prev;
			if (item->prev)	item->prev->next = item->next;
			free(item);
			elem_list->count--;
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

