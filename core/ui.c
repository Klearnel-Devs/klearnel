/*
 * Contains all routines to execute actions linked to args in cli
 * 
 * Copyright (C) 2014, 2015 Klearnel-Devs
 */
#include <global.h>
#include <core/ui.h>

/* Main function that calls the right routine 
 * acordingly to the passed arguments
 */ 
void execute_commands(int nb, char **commands)
{
	if (!strcmp(commands[2], "add-to-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel add-to-qr <filename or dirname>\n");
		}
		NOT_YET_IMP;
	} else if (!strcmp(commands[2], "del-from-qr")) {
		if (nb < 3) {
			fprintf(stderr, "Element to add missing\n"
				 "Correct syntax is: klearnel del-fom-qr <filename or dirname>\n");
		}
		NOT_YET_IMP;
	} else if (!strcmp(commands[2], "get-qr-list")) {
		NOT_YET_IMP;
	} else if (!strcmp(commands[2], "-h")) {
		NOT_YET_IMP;
	} else {
		fprintf(stderr, "Unknow command\n"
			"Try klearnel -h to get a complete list of available commands\n");
		exit(EXIT_FAILURE);
	}
}
