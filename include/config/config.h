/*-------------------------------------------------------------------------*/
/**
   \file	config.h
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	Configuration management header

   This file implements the iniparser library to manage configuration
   files for Klearnel.
*/
/*--------------------------------------------------------------------------*/


#ifndef _CONFIG_H
#define _CONFIG_H

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/

#include <global.h>
#include <logging/logging.h>
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>   

/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/


 #define CONFIG		BASE_DIR "/conf"
 #define DEF_CFG	CONFIG "/klearnel.conf"
 #define CFG_TMP	TMP_DIR "/conf"

 #define MAX	100

/*---------------------------------------------------------------------------
                                New types
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  \brief	Section Keys Structure

  Structure containing the amount of keys and corresponding
  key names in configuration ini file for given section

 */
/*-------------------------------------------------------------------------*/
typedef struct sectionkeys {
	int num;       //!< Number of keys
	char** names;  //!< Names of each key
} TKeys;

/*-------------------------------------------------------------------------*/
/**
  \brief	Section Structure

  Structure containing the name of a section and a pointer
  to the next section

 */
/*-------------------------------------------------------------------------*/
typedef struct section {
	char *section_name;    //!< Name of the section
	struct section *next;  //!< Pointer to next section struct
	TKeys *keys;           //!< Pointer to sectionkeys structure
} TSection;

/*-------------------------------------------------------------------------*/
/**
  \brief	Linked List Head

  Structure for linked list of configuration module, counting the
  number of sections with a pointer to the 'first'

 */
/*-------------------------------------------------------------------------*/
typedef struct sectionList {
	int count;             //!< Number of sections
	struct section* first; //!< Pointer to first section structure
} TSectionList;

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief	Initializes Configuration Module
  \return	void

  Creates the configuration folder if not already created
  Creates the temporary configuration folder if not already created
  Creates the default configuration file if not already created
  Loads configuration file & calls function to populate list
  All errors save CFG_TMP creation results in EXIT_FAILURE
 */
/*--------------------------------------------------------------------------*/
void init_config();

/*-------------------------------------------------------------------------*/
/**
  \brief	Gets corresponding configuration value for section:key
  \param	section 	Name of section in ini file
  \param	key 		Name of key in ini file
  \return	Returns the corresponding value of the section:key

  Gets corresponding configuration value for section:key passed 
  as parameters
 */
/*--------------------------------------------------------------------------*/
const char * get_cfg(char *section, char *key);

/*-------------------------------------------------------------------------*/
/**
  \brief	Modifies value of corresponding section:key
  \param	section 	Name of section in ini file
  \param	key 		Name of key in ini file to modify
  \param	value		Value to inject
  \return	0 if OK, -1 otherwise

  Modifies value of corresponding section:key passed as parameters
  to new value
 */
/*--------------------------------------------------------------------------*/
int modify_cfg(char *section, char *key, char *value);

/*-------------------------------------------------------------------------*/
/**
  \brief	Dumps configuration to indicated filepath
  \param	filepath	
  \return	0 if OK, -1 otherwise

  Dumps configuration to indicated filepath, if no filepath is
  indicated, dumps to the temporary config folder in /tmp/.klearnel
 */
/*--------------------------------------------------------------------------*/
int dump_cfg(char* filepath);

/*-------------------------------------------------------------------------*/
/**
  \brief	Free's the configuration linked list & unloads the
  		dictionary
  \return	void

  Free's each allocated pointer in the linked list then free's
  the dictionary
 */
/*--------------------------------------------------------------------------*/
void free_cfg();

#endif /* _CONFIG_H */