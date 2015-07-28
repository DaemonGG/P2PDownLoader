#include "win_log.h"

static FILE* _winlog;
static struct timeval start_tm;
void init_winlog(char * logfile)
{
	gettimeofday(&start_tm,NULL);

	_winlog=fopen(logfile,"a");
	if(_winlog==NULL)
	{
		printf("Init window log failed!!\n");
		exit(0);
	}
	return;
}
void winlog(const char *format, ...)
{
	va_list args;
    va_start(args, format);
    vfprintf(_winlog, format, args);
    fflush(_winlog);
    va_end(args);
}

void WSize_log(int winid,double winsize)
{
	struct timeval curr_tm;
	gettimeofday(&curr_tm,NULL);
	int _millisec=1000*(curr_tm.tv_sec - start_tm.tv_sec)
				+(curr_tm.tv_usec - start_tm.tv_usec)/1000;
	winlog("f%d\t%d\t%f\n",winid,_millisec,winsize);
}

void W_verb_log(int winid,double winsize,int threshold,int LPA,int LPS,int LPR,int count,char *timeout_reason)
{
	struct timeval curr_tm;
	gettimeofday(&curr_tm,NULL);
	int _millisec=1000*(curr_tm.tv_sec - start_tm.tv_sec)
				+(curr_tm.tv_usec - start_tm.tv_usec)/1000;

	printf("W%d\t%d\t%f\t%d\t%d\t%d\t%d\t%d\t%s\n",winid,_millisec,winsize,threshold,LPA,LPS,LPR,count,timeout_reason);
}

int get_id_bytime()
{
	struct timeval curr_tm;
	gettimeofday(&curr_tm,NULL);
	int usec=1000000*(curr_tm.tv_sec - start_tm.tv_sec)
				+(curr_tm.tv_usec - start_tm.tv_usec);

	return usec;
}