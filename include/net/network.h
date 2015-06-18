 /**
 * \file network.h 
 * \brief Network header
 * \author Copyright (C) 2014, 2015 Klearnel-Devs
 * 
 * Contains all prototypes and structures used 
 * by the network feature
 * 
 * 
 */
#ifndef _KLEARNEL_NETWORK_H
#define _KLEARNEL_NETWORK_H
/*---------------------------------------------------------------------------
                                Definitions
 ---------------------------------------------------------------------------*/
/**
  \brief The token file in Klearnel base folder
 */
#define TOKEN_DB BASE_DIR "/klearnel.tk" 
/**
  \brief The "get configuration list" signal
 */
#define CONF_LIST 20
/**
  \brief The "modify configuration item" signal
 */
#define CONF_MOD 21
 /**
  \brief The simple connection signal
 */
#define NET_CONNEC 30

/*---------------------------------------------------------------------------
                                Prototypes
 ---------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/**
 \brief Execute the action received through the network socket
 \param buf the buffer received previously in the network socket
 \param c_len the length of the command 
 \param action the action to execute
 \param s_cl the client socket to send result or receive other information
 \return Return 0 on success and -1 on error

 This function calls another function following the action to perform:
 - If it is a Quarantine action, it calls: _execute_qr_action(...)
 - If it is a Scanner action, it calls: _execute_scan_action(...)

*/
/*-------------------------------------------------------------------------*/
int execute_action(const char *buf, const int c_len, const int action, const int s_cl);
/*-------------------------------------------------------------------------*/
/**
 \brief Global function of the Tasker
 \return void

 This function waits continously for an action to execute from the network socket.
 When an action is received, it calls execute_action to perform the corresponding action.

*/
/*-------------------------------------------------------------------------*/
void networker();
/*-------------------------------------------------------------------------*/
/**
 \brief Generate the Klearnel token at first launch
 \return void

 This function generates an random unique token starting with KL and completed 
 with numbers. It asks the user for a path where a copy of this token is stored
 allowing him to use the token in Klearnel Manager

*/
/*-------------------------------------------------------------------------*/
int generate_token();

#endif
