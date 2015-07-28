#ifndef WIN_LOG_H
#define WIN_LOG_H

#include <stdio.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
void init_winlog(char * logfile);
void winlog(const char *format, ...);
void WSize_log(int winid,double winsize);
void W_verb_log(int winid,double winsize,int threshold,int LPA,int LPS,int LPR,int count,char *timeout_reason);
int get_id_bytime();
#endif