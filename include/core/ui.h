/*-------------------------------------------------------------------------*/
/**
   \file	ui.h
   \author	Copyright (C) 2014, 2015 Klearnel-Devs 
   \brief	User interface header

   
*/
/*--------------------------------------------------------------------------*/
#ifndef _KLEARNEL_UI_H
#define _KLEARNEL_UI_H


/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/
#define PID_MAX_S sizeof(pid_t)

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief	Allow to send a query to the quarantine
  \param 	nb 		Number of commands
  \param 	commands 	Commands to execute
  \param 	action 		The action to take
  \return	0 on success, -1 on error	

  
 */
/*--------------------------------------------------------------------------*/
int qr_query(int nb, char **commands, int action);

/*-------------------------------------------------------------------------*/
/**
  \brief	Allow to send a query to the scanner
  \param 	nb 		Number of commands
  \param 	commands 	Commands to execute
  \param 	action 		The action to take
  \return	0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int scan_query(int nb, char **commands, int action);

/*-------------------------------------------------------------------------*/
/**
  \brief	Function that executes commands
  \param	nb 		The number of commands
  \param 	commands 	The commands to execute
  \return	void

  Main function that calls the right routine 
  according to the passed arguments
 */
/*--------------------------------------------------------------------------*/
void execute_commands(int nb, char **commands);

#endif /* _KLEARNEL_UI_H */