#ifndef _CONFIG_H
#define _CONFIG_H
/*
 * Configuration management header
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 *
 */

typedef struct config {
	char* section;
	char* key;
	char* value;
} TConfig;



#endif /* _CONFIG_H */