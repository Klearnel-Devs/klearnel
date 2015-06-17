/*-------------------------------------------------------------------------*/
/**
   \file	config.c
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Configuration management file

   This file implements the iniparser library to manage configuration
   files for Klearnel.
*/
/*--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include <global.h>
#include <logging/logging.h>
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>
#include <config/config.h>

/*---------------------------------------------------------------------------
                                Global Variables
 ---------------------------------------------------------------------------*/
 static dictionary *ini = NULL;
 static TSectionList *section_list = NULL;

/*-------------------------------------------------------------------------*/
/**
  \brief	Creates default configuration
  \return	0 if OK, -1 otherwise

  Creates the default configuration INI file
 */
/*--------------------------------------------------------------------------*/
int _create_cfg()
{
	FILE *ini;

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
	"[SMALL]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= 0 ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n"
	"[MEDIUM]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= 0 ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n"
	"[LARGE]\n"
	"EXP_DEF	= 2592000 ;\n"
	"BACKUP		= 0 ;\n"
	"LOCATION	= No Location Specified ;\n"
	"\n");

	fclose(ini);

	return 0;
	err:
		write_to_log(FATAL, "Could not create/open log file");
		return -1;
}

/*-------------------------------------------------------------------------*/
/**
  \brief	Retrieves # of keys and their names for a given section
  \param 	section 	Structure of type TSection
  \return	void

  Creates a structure of type TKeys, retrieves the # of keys &
  corresponding key names
 */
/*--------------------------------------------------------------------------*/
void _get_keys(TSection *section)
{
	TKeys *keys = malloc(sizeof(struct sectionkeys));
	if ((keys->num = iniparser_getsecnkeys(ini, section->section_name)) == -1){
		write_to_log(WARNING, "%s - %d - %s", __LINE__, __func__, "Unable to count config section key");
		return;
	}
	keys->names = iniparser_getseckeys(ini, section->section_name);
	section->keys = keys;
	return; 
}

/*-------------------------------------------------------------------------*/
/**
  \brief	Gets # of sections and their names
  \return	0 if OK, -1 otherwise

  Populates the structure of type TSection and TSectionList
 */
/*--------------------------------------------------------------------------*/
int _get_sections()
{
	int i;
	if ((section_list->count = iniparser_getnsec(ini)) == -1){
		write_to_log(WARNING, "%s - %d - %s", __LINE__, __func__, "Unable to count config sections");
		return -1;
	}
	for(i = 0; i < section_list->count; i++) {
		TSection* node = malloc(sizeof(struct section));
		node->section_name = iniparser_getsecname(ini, i);
		_get_keys(node);
		if (section_list->first == NULL) {
			section_list->first = node;
		} else {
			TSection *tmp = section_list->first;
			section_list->first = node;
			section_list->first->next = tmp;
		}
	}
	return 0; 
}

void init_config()
{
	if (access(CONFIG, F_OK) == -1) {
		if (mkdir(CONFIG, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
			write_to_log(FATAL, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the config folder", CONFIG);
			goto err;
		}
		if (_create_cfg() == -1){
			write_to_log(FATAL, "%s", "Default configuration file is missing and could not be created");
			goto err;
		}
	}
	if (access(CFG_TMP, F_OK) == -1) {
		int oldmask = umask(0);
		if (mkdir(CFG_TMP, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			write_to_log(WARNING, "%s - %d - %s - %s", __func__, __LINE__, "Unable to create the temp config folder", CFG_TMP);
			return;
		}
		umask(oldmask);	
	}
	if (access(DEF_CFG, F_OK) == -1) {
		if (_create_cfg() == -1){
			write_to_log(FATAL, "%s", "Default configuration file is missing and could not be created");
			goto err;
		}
	}
	ini = iniparser_load(DEF_CFG);
	if (ini==NULL) {
		write_to_log(FATAL, "cannot parse file: %s\n", DEF_CFG);
		goto err;
	}
	if (!section_list) {
		section_list = malloc(sizeof(struct sectionList));
		if(!section_list) {
			write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
			goto err;
		}
	}
	if (_get_sections() != 0) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Section List could not be filled");
		goto err;
	}
	write_to_log(INFO, "%s", "Configuration initialized without error");
	return;
	err:
		exit(EXIT_FAILURE);
}

int dump_cfg(char* filepath) {
	FILE *tmp_cfg;
	int oldmask = umask(0);
	if (filepath == NULL) {
		char *tmp = malloc(strlen(CFG_TMP)+strlen("/tmp.conf") + 1);
		if (!snprintf(tmp, strlen(CFG_TMP)+strlen("/tmp.conf") + 1, "%s%s", CFG_TMP, "/tmp.conf")) {
			LOG(WARNING, "Unable to create tmp config filepath");
			free(tmp);
			goto err;
		}
		if ((tmp_cfg = fopen(tmp, "w")) == NULL) {
			umask(oldmask);
			goto err;
		}
		free(tmp);
	} else {
		if ((tmp_cfg = fopen(filepath, "w")) == NULL) {
			umask(oldmask);
			goto err;
		}
	
	}
	umask(oldmask);
	iniparser_dump(ini, tmp_cfg);
	fclose(tmp_cfg);
	return 0;
err:
	write_to_log(WARNING, "Unable to dump config file");
	return -1;
}

void free_cfg()
{
	int i;
	TSection* node = malloc(sizeof(struct section));
	for(i = 0; i < section_list->count; i++) {
		node = section_list->first->next;
		free(section_list->first);
		section_list->first = node;
	}
	section_list->count = 0;
	iniparser_freedict(ini);
}

char * get_cfg(char *section, char *key) {
	char *value;
	char *query = malloc(strlen(section) + strlen(key) + strlen(":") + 1);
	if (!snprintf(query, (strlen(section) + strlen(key) + strlen(":") + 1), "%s:%s", section, key)) {
		write_to_log(WARNING, "%s - %d - %s - %s:%s", __LINE__, __func__, "Unable to set config query for", section, key);
		goto err;
	}

	if ((value = iniparser_getstring(ini, query, NULL)) == NULL) {
		write_to_log(WARNING, "%s - %d - %s - %s:%s", __LINE__, __func__, "Unable to get config value for", section, key);
	}
	free(query);
	return value;
err:
	free(query);
	return NULL;
}

int modify_cfg(char *section, char *key, char *value)
{
	char *query = malloc(strlen(section) + strlen(key) + strlen(":") + 1);
	if (section == NULL) {
		write_to_log(WARNING, "Unable to modify config, parameter is NULL - Section");
		goto err;
	} else if (key == NULL) {
		write_to_log(WARNING, "Unable to modify config, parameter is NULL - Key");
		goto err;
	} else if (value == NULL) {
		write_to_log(WARNING, "Unable to modify config, parameter is NULL - Value");
		goto err;
	}
	if (!snprintf(query, (strlen(section) + strlen(key) + strlen(":") + 1), "%s:%s", section, key)) {
		write_to_log(WARNING, "%s - %d - %s - %s:%s", __LINE__, __func__, "Unable to set config query for", section, key);
		goto err;
	}
	if (iniparser_set(ini, query, value) != 0) {
		write_to_log(WARNING, "Unable to modify config");
		goto err;
	}
	free(query);
	return 0;
err:
	free(query);
	return -1;
}

void save_conf() 
{
	unlink(DEF_CFG);
	FILE *conf_f = fopen(DEF_CFG, "w");
	if (!conf_f) {
		LOG(URGENT, "Unable to open the config file");
		return;
	}

	iniparser_dump_ini(ini, conf_f);
	fclose(conf_f);
}

void reload_config() 
{
	free_cfg();
	ini = iniparser_load(DEF_CFG);
	if (ini==NULL) {
		write_to_log(FATAL, "cannot parse file: %s\n", DEF_CFG);
		goto err;
	}

	if (!section_list) {
		section_list = malloc(sizeof(struct sectionList));
		if(!section_list) {
			write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Unable to allocate memory");
			goto err;
		}
	}

	if (_get_sections() != 0) {
		write_to_log(FATAL, "%s - %d - %s", __func__, __LINE__, "Section List could not be filled");
		goto err;
	}

	write_to_log(INFO, "%s", "Configuration reloaded without error");

	return;
	err:
		exit(EXIT_FAILURE);
}