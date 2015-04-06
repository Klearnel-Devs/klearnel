#ifndef _CONFIG_H
#define _CONFIG_H
/*
 * Configuration management header
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 *
 */

 #define PROFILES	BASE_DIR "/profiles"
 #define CFG_SOCK	TMP_DIR "/kl-cfg-sck"
 #define CFG_TMP	TMP_DIR "/config"

 #define EXP_DEF	2592000

/* Structure of files */

typedef struct config {
	char* section;
	char* key;
	char* value;
} TConfig;

/*----- PROTOYPE ------ */

void cfg_worker();
void init_config();
#endif /* _CONFIG_H */