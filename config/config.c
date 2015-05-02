 /*
 * Contains all functions required to maintain 
 configuration profiles
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */

#include <global.h>
#include <config/config.h>
#include <logging/logging.h>
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>

 static dictionary *ini ;

int _create_cfg()
{
	FILE    *   ini ;

	if ((ini = fopen(DEF_CFG, "w")) == NULL)
		goto err;
	fprintf(ini,
	"#\n"
	"# This is the Klearnel configuration ini file\n"
	"#\n"
	"\n"
	"[GLOBAL]\n"
	"LOG_AGE    	= 2592000 ;\n"
	"SMALL 		= 1048576 ;\n"
	"MEDIUM		= 10485760 ;\n"
	"LARGE		= 104857600 ;\n"
	"\n"
	"[SCANNER]\n"
	"\n"
	"[SMALL]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= FALSE ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n"
	"[MEDIUM]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= FALSE ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n"
	"[LARGE]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= FALSE ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n");

	fclose(ini);

	return 0;
	err:
		write_to_log(FATAL, "Could not create/open log file");
		return -1;
}

void init_config()
{  
	if (access(CONFIG, F_OK) == -1) {
		if (mkdir(CONFIG, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the config folder", CONFIG);
			return;
		}
		if (_create_cfg() == -1)
			goto err;
	}
	if (access(CFG_TMP, F_OK) == -1) {
		if (mkdir(CFG_TMP, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the temp config folder", CFG_TMP);
			return;
		}
	}
	if (access(DEF_CFG, F_OK) == -1) {
		if (_create_cfg() == -1)
			goto err;
	}
	ini = iniparser_load(DEF_CFG);
	if (ini==NULL) {
		write_to_log(FATAL, "cannot parse file: %s\n", DEF_CFG);
		return;
	}
	write_to_log(INFO, "%s", "Configuration Initialized without error");
	return;
	err:
		write_to_log(FATAL, "%s", "Default configuration file is missing and could not be created");

}