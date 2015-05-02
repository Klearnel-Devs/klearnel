#ifndef _CONFIG_H
#define _CONFIG_H
/*
 * Configuration management header
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 *
 */

 #define CONFIG		BASE_DIR "/conf"
 #define DEF_CFG	CONFIG "/klearnel.conf"
 #define CFG_TMP	TMP_DIR "/conf"

 #define MAX	100

typedef struct sectionkeys {
	int num;
	char** names;
} TKeys;

typedef struct section {
	char *section_name;
	struct section *next;
	TKeys *keys;
} TSection;

typedef struct sectionList {
	int count;
	struct section* first;
} TSectionList;

/*----- PROTOYPE ------ */
void init_config();
const char * get_cfg(char *section, char *key);
int modify_cfg(char *section, char *key, char *value);
int dump_cfg(char* filepath);
void free_cfg();

#endif /* _CONFIG_H */