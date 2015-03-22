#ifndef _KLEARNEL_UI_H
#define _KLEARNEL_UI_H
/*
 * User interface header
 * Copyright (C) 2014, 2015 Klearnel-Devs
 *
 */

#define PID_MAX_S sizeof(pid_t)

int qr_query(int nb, char **commands, int action);
int scan_query(int nb, char **commands, int action);

void execute_commands(int nb, char **commands);

#endif /* _KLEARNEL_UI_H */