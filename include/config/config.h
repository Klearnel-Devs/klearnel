#ifndef _CONFIG_H
#define _CONFIG_H
/*
 * Configuration management header
 *
 * Copyright (C) 2014, 2015 Klearnel-Devs 
 *
 */

 #define CONFIG		BASE_DIR "/config"
 #define DEF_CFG	CONFIG "/klearnel.conf"
 #define CFG_TMP	TMP_DIR "/config"

 #define MAX	100	

/*----- PROTOYPE ------ */

void init_config();
#endif /* _CONFIG_H */