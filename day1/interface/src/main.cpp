/*
 * main.cpp
 *
 *  Created on: 2009-7-28
 *      Author: Administrator
 */

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <errno.h>
#include <sys/wait.h>
#include <ucontext.h>
#include "main.hpp"
#include "share.hpp"
using namespace std;
#include "command.hpp"
#include "procd.hpp"

#ifndef min
#define min(a,b) ((a < b) ? a :b)
#endif

#include <signal.h>
#include <execinfo.h>

__pid_t iPID=-1;
command *g_pParser=NULL;
void exit_func(int sign_no)
{
	syslog(LOG_LOCAL7|LOG_ERR,"-interface- I have get SIG %d\n",sign_no);
	if (sign_no==SIGTERM && iPID == getpid())
	{
		g_pParser->stopSubProcess();
	}

	//wait all sub process exit
	while(wait(NULL)!=-1){}

	if (g_pParser)
	{
		delete g_pParser;
		g_pParser=NULL;
	}

    exit(0);
}

void dump_func(int signo)
{
    void *array[20];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 20);
    strings = backtrace_symbols (array, size);

    syslog(LOG_LOCAL7|LOG_ERR,"=========Process run error=========\n");
    syslog(LOG_LOCAL7|LOG_ERR,"\nError signle %d\n", signo);
    syslog(LOG_LOCAL7|LOG_ERR,"Obtained %zd stack frames.\n", size);
    for (i = 0; i <size; i++)
        syslog(LOG_LOCAL7|LOG_ERR,"%s\n", strings[i]);
    syslog(LOG_LOCAL7|LOG_ERR,"==========-=====end================\n");
    //fprintf (stderr,"\nError signle %d\n", signo);
    //fprintf (stderr,"Obtained %zd stack frames.\n", size);
    //for (i = 0; i <= size; i++)
     //   fprintf (stderr,"%s\n", strings[i]);

    free (strings);

    exit_func(0);
    exit(0);
}

void myhandler (int sn , siginfo_t  *si , void* context)
{
	syslog(LOG_LOCAL7|LOG_ERR,"signal number = %d, signal errno = %d, signal code = %d\n",
		si->si_signo,si->si_errno,si->si_code);
	syslog(LOG_LOCAL7|LOG_ERR," senders' pid = %d, sender's uid = %d, \n",si->si_pid,si->si_uid);

	exit_func(si->si_signo);
}

void set_dump_func(void)
{
    signal(SIGSEGV, dump_func);
    signal(SIGABRT, dump_func);
    signal(SIGFPE, dump_func);
    signal(SIGPIPE, dump_func);
    signal(SIGILL, dump_func);
}

void set_exit_func(void)
 {
    signal(SIGINT, exit_func);
    signal(SIGQUIT, exit_func);
    signal(SIGKILL, exit_func);
    //signal(SIGABRT, exit_func);

	struct sigaction s;
	s.sa_flags = SA_SIGINFO;
	s.sa_sigaction = myhandler;
	sigaction (SIGTERM,&s,(struct sigaction *)NULL);

    //若父进程退出，退出该进程
    signal(SIGHUP, exit_func);
    prctl(PR_SET_PDEATHSIG,SIGHUP);

    set_dump_func();
}


void *msg_sata_thread(void *p)
{
    int qid=*(int *)p;
    struct msqid_ds qbuf;
    MSG_BUFF msg;
    while(qid>=0)
    {
            if( msgctl(qid, IPC_STAT, &qbuf) == 0)
            {
                //syslog(LOG_LOCAL7|LOG_INFO,"msg_qnum %ld, msg_qbytes %ld, __msg_cbytes %ld\n",qbuf.msg_qnum,qbuf.msg_qbytes,qbuf.__msg_cbytes);
                if(qbuf.msg_qnum>1000||(qbuf.__msg_cbytes+512)>=16384)
                {
                    while (msgrcv(qid, &msg, MAX_MSG_SIZE, 0, IPC_NOWAIT)>=0);//clear
					while (msgrcv(qid, &msg, MAX_MSG_SIZE, PROC_INTERFACE, IPC_NOWAIT)>=0);//clear
                    syslog(LOG_LOCAL7|LOG_INFO,"clear message in buffer: %ld, cbytes %ld\n",qbuf.msg_qnum,qbuf.__msg_cbytes);
                }
            }else{
                syslog(LOG_LOCAL7|LOG_ERR,"msgctl error = %d,%s.",errno,strerror(errno));
            }
           usleep(100000);
    }
    syslog(LOG_LOCAL7|LOG_ERR,"msg_sata_thread: qid %d error\n",qid);
    return 0;
}

void msg_sata(int *qid)
{
    pthread_t thread;
    if(pthread_create(&thread, NULL, msg_sata_thread, qid)!=0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"msg_sata() pthread create error");
        //exit(3);
    }
}

int main(int argc, char** argv)
{
    //In the main function, we reads the system pipeline to get commands
    //Command will be assign with the process identifier in order to return
    //   to the specific process
    //In single module testing, command is sent from stdin and return to stdout

    int qid, len;
    MSG_BUFF msg;
    string tempStr;
    openlog("INTERFACE",LOG_CONS | LOG_PID|LOG_PERROR, LOG_LOCAL7);
    set_exit_func();
    signal(SIGCHLD, SIG_IGN) ;//避免僵尸进程
    int gpsboard=0;
    if(argc>1)
    {
        if(stristr(argv[1],"NOVATEL")!=NULL)
        {
            gpsboard=GPS_NOVATEL;
        }
        else if(stristr(argv[1],"TRIMBLE")!=NULL)
        {
            gpsboard=GPS_TRIMBLE;
        }
        else if(stristr(argv[1],"UNICORE")!=NULL)
        {
            gpsboard=GPS_UNICORECOMM;
        }
        else if(stristr(argv[1],"HEMISPHERE")!=NULL)
        {
            gpsboard=GPS_HEMISPHERE;
        }
    }

    //command parser(gpsboard);
	g_pParser= new command(gpsboard);
    if ((qid = msgget(KEY_MSGQUEUE, IPC_CREAT | 0666)) == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"creat message queue error");
        exit(1);
    }
    g_pParser->interface_qid = qid;
    //syslog(LOG_INFO,"Message queue id is %d",qid);
    iPID = getpid();
    syslog(LOG_LOCAL7|LOG_INFO,"pid %d\n",iPID);

    //msgctl(qid, IPC_RMID, NULL); //ipcrm -Q keyid
	//while ((len = msgrcv(qid, &msg, 512, thisProc, IPC_NOWAIT)) >=0);
//parser.get_infos();
#ifdef DEBUG
   //parser.init_gps();
#else
    if(!g_pParser->init_gps())
    {
        printf("init gps error\n");
        syslog(LOG_LOCAL7|LOG_ERR,"init gps error");
    }
    g_pParser->init_oled();
    g_pParser->init_dev();

#endif
    msg_sata(&qid);
    procd(g_pParser);
    while (msgrcv(qid, &msg, MAX_MSG_SIZE, PROC_INTERFACE, IPC_NOWAIT)>=0);//clear
	printf("interface: start cmd loop\n");
    while (1)
    {
        if ((len = msgrcv(qid, &msg, MAX_MSG_SIZE, PROC_INTERFACE, 0)) < 0)
        {
            syslog(LOG_LOCAL7|LOG_ERR,"read message error!");
            exit(1);
        }
        msg.msg_text[len-4] = 0x0;
        //printf("total len: %d type %ld from %08x message %s \n", len, msg.msg_type, msg.msg_source, msg.msg_text);
        tempStr.assign(msg.msg_text);

        //tempStr.assign("GET,Network\\WAN\\IP\r\n");
        g_pParser->CMDParse(tempStr,msg.msg_source);

        msg.msg_type = msg.msg_source;
        msg.msg_source = PROC_INTERFACE;
		//lrd_test
        //cout << "RETURN: " << g_pParser->returnMsg << endl;
        //strncpy(msg.msg_text,parser.returnMsg.c_str(),parser.returnMsg.length());

        int i,n,m,size,writen;
        const char *p=g_pParser->returnMsg.c_str();
        size=g_pParser->returnMsg.length();
        m=size/1000;
        writen=0;

        for(i=0;i<=m;i++)
        {
            n=min(size-writen,1000);
            strncpy(msg.msg_text,p+writen,n);
            msg.msg_text[n]=0;
            len = n+ 5;
            if ((msgsnd(qid, &msg, len, 0)) < 0)
            {
                printf("qid: %d len:%d\n",qid, len);
				syslog(LOG_LOCAL7|LOG_ERR,"[msgsnd]ERROR: errno = %d, strerror = %s \n" , errno, strerror(errno));
				//perror("interface send message error\n") ;
                //exit(1);
            }
            writen+=n;
        }
		//printf("##i:%d VS m:%d len %d\r\n",i,m,len);

    }

    closelog();
    return 0;
}

