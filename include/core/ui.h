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
/*-------------------------------------------------------------------------*/
/**
  \brief  Get the maximum size of a type pid_t
 */
/*-------------------------------------------------------------------------*/
#define PID_MAX_S sizeof(pid_t)

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
  \brief  Send KL_EXIT to all Klearnel services
  \return 0 on success, -1 if Klearnel does not shutdown successfully 

  Ask to all Klearnel services to shutdown with the KL_EXIT signal.
  
 */
/*--------------------------------------------------------------------------*/
int stop_action();

/*-------------------------------------------------------------------------*/
/**
  \brief	Allow to send a query to the quarantine
  \param 	commands 	Commands to execute
  \param 	action 		The action to take
  \return	0 on success, -1 on error	

  
 */
/*--------------------------------------------------------------------------*/
int qr_query(char **commands, int action);

/*-------------------------------------------------------------------------*/
/**
  \brief	Allow to send a query to the scanner
  \param 	commands 	Commands to execute
  \param 	action 		The action to take
  \return	0 on success, -1 on error

  
 */
/*--------------------------------------------------------------------------*/
int scan_query(char **commands, int action);

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