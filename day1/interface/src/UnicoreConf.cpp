/*
 * NovConf.cpp
 *
 *  Created on: 2009-8-29
 *      Author: zekunyao
 */
#include <string.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "UnicoreConf.hpp"
#include "share.hpp"

#define ENABLE_SAVE
using namespace std;

static char OEM_NMEA_TRG[][16] = {"ONTIME","ONCHANGED","ONCE"};
static char OEM_NMEA_MSG[][16] = {"GPGGA","GPGSA","GPGSV","GPZDA","GPGST","GPGLL","GPRMC","GPVTG","GPGGQ","GPGGK","GPGNS","LOGLISTA","COMCONFIGA","BESTPOSA"};

cUnicoreCommand::cUnicoreCommand() {
	if ((cmdmailbox = msgget(KEY_CMDMAILBOX, IPC_CREAT |0666)) == -1)
    {
        //perror("creat message queue erro");
        syslog(LOG_LOCAL7|LOG_ERR,"creat mailbox error");
        //exit(1);
    }
    key_t key;
    key=IPC_GPS_CONF_WRITE;
    //printf("id %d\n",key);
	{
		if (AllocSHM(&m_IPCGPSRAWWrite,key,IPC_GPS_CONF_WRITE_SIZE8K)!=0)
		{
			syslog(LOG_LOCAL7|LOG_ERR,"AllocSHM  %d error",key);
		}

		else FIFOInitForWriting(&(m_IPCGPSRAWWrite.IPCFIFO),FIFOWrite_SIZE(IPC_GPS_CONF_WRITE_SIZE8K));
	}
	//lrd_test
	printf("cUnicoreCommand: cmd fifo id:%d\n",key);
    base_started=0;
    error_count=0;
}

cUnicoreCommand::~cUnicoreCommand() {

	//lrd_test
	if(FreeSHM(&m_IPCGPSRAWWrite)!=0)
	{
		printf("cUnicoreCommand: free fifo id:%d failed\n",m_IPCGPSRAWWrite.iIPCMId);
	}
}

bool cUnicoreCommand::getResult() {
	return result;
}

void cUnicoreCommand::resetResult() {
	result = true;
}

bool cUnicoreCommand::enableallprn()
{
	return true;
}

bool cUnicoreCommand::disableprn(int prn)
{
	return true;
}

bool cUnicoreCommand::sendcmd(string cmd) {
	//bool result = true;
	return sendOnce(cmd);
}

bool cUnicoreCommand::sendOnce(string cmd) {
	int i;
	int len;
	string command(cmd);
	command.append("\r\n");
	len = command.length() ;
	while(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0);//clear
	//syslog(LOG_LOCAL7|LOG_INFO,"oem cmd: %s",command.c_str());
    FIFO_Add_Buffer(&(m_IPCGPSRAWWrite.IPCFIFO),(unsigned char *)command.c_str(),len);
	for(i=0;i<10;i++)
	 {
        usleep(1000*100);//sleep(1);
        if(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0)
        {
            error_count=0;
            return true;
        }
	 }
    //printf("cmd timeout\n");
    syslog(LOG_LOCAL7|LOG_ERR,"oem cmd error: %s",command.c_str());
    error_count++;
    return false;
}

bool cUnicoreCommand::startbase(double lat, double lon, float height, int id)
{
    string str;
    char tempStr[128];

	sendcmd("UNDULATION USER 0.0");

	sprintf(tempStr,"FIX POSITION %.9lf %.9lf %.3f",lat,lon,height);
    str.assign(tempStr);
    cout<< str<<endl;
     if (!sendcmd(str))
		return false;
    base_started=1;
	return true;
}

bool cUnicoreCommand::stopbase()
{
	sendcmd("undulation egm96");
     if (!sendcmd("fix none"))
		return false;
    base_started=0;
#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}


bool cUnicoreCommand::checkbasestarted()
{
    if(base_started) return true;
    return false;
}


bool cUnicoreCommand::issueRTCM23(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
/*
INTERFACEMODE %s NOVATEL RTCM ON
GENERATERTKCORRECTIONS RTCM %s
*/
	stopDiff(port);

    sprintf(tempStr,"INTERFACEMODE %s UNICORE RTCM ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sprintf(tempStr,"LOG %s RTCM1819 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) return false;

    sprintf(tempStr,"LOG %s RTCM3 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM1 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM9 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

	sprintf(tempStr,"LOG %s RTCM31 ONTIME 1",portStr);
	str.assign(tempStr);
	if (!sendcmd(str)) ;

	sprintf(tempStr,"LOG %s RTCM41 ONTIME 1",portStr);
	str.assign(tempStr);
	if (!sendcmd(str)) ;

	sprintf(tempStr,"LOG %s RTCM42 ONTIME 1",portStr);
	str.assign(tempStr);
	if (!sendcmd(str)) ;

#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueRTCM30(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
/*
INTERFACEMODE %s NOVATEL RTCMV3 ON
GENERATERTKCORRECTIONS RTCMV3 %s
*/
	stopDiff(port);

    sprintf(tempStr,"INTERFACEMODE %s UNICORE RTCMV3 ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"LOG %s RTCM1006 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) return false;

    sprintf(tempStr,"LOG %s RTCM1007 ONTIME 15",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM1004 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM1012 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM1104 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

    sprintf(tempStr,"LOG %s RTCM1033 ONTIME 15",portStr);
    str.assign(tempStr);
    if (!sendcmd(str)) ;

#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueRTCMV3EPH(unsigned char port, string rtcmv3_eph)
{
    return true;
}

bool cUnicoreCommand::issueRTCM32(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
/*
INTERFACEMODE %s NOVATEL RTCMV3 ON
GENERATERTKCORRECTIONS RTCMV3 %s
*/
	stopDiff(port);

    sprintf(tempStr,"INTERFACEMODE %s UNICORE RTCMV3 ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"LOG %s RTCM1075 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1085 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

   sprintf(tempStr,"LOG %s RTCM1125 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1005 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1007 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1019 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1020 ONTIME 1",portStr);//glonass
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM63 ONTIME 1",portStr);//beidou
    str.assign(tempStr);
    if (!sendcmd(str));

#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueRTCM3210(unsigned char port, int id)
{
	string str;
	char tempStr[128];
    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

    sprintf(tempStr,"unlogall %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"INTERFACEMODE %s UNICORE RTCMV3 ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;
    /*
    LOG  RTCM1074 ONTIME 10
    LOG  RTCM1084 ONTIME 10
    LOG  RTCM1124 ONTIME 10
    LOG  RTCM1019 ONTIME 10
    LOG  RTCM1020 ONTIME 10
    */

    sprintf(tempStr,"LOG %s RTCM1074 ONTIME 10",portStr);
	str.assign(tempStr);
    if (!sendcmd(str));
    {
        if (!sendcmd(str));
        //return false;
    }

    sprintf(tempStr,"LOG %s RTCM1084 ONTIME 10",portStr);
	str.assign(tempStr);
    if (!sendcmd(str));
        //return false;

    sprintf(tempStr,"LOG %s RTCM1124 ONTIME 10",portStr);
	str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"LOG %s RTCM63 ONTIME 10",portStr);
	str.assign(tempStr);
	if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1019 ONTIME 10",portStr);
	str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1020 ONTIME 10",portStr);
	str.assign(tempStr);
    if (!sendcmd(str));
#ifdef ENABLE_SAVE
	sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueROX(unsigned char port, int id)
{
    syslog(LOG_LOCAL7|LOG_ERR,"init rox failed.");
    return false;
}

bool cUnicoreCommand::issueCMR(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

/*
INTERFACEMODE %s NOVATEL CMR ON
GENERATERTKCORRECTIONS CMR %s
*/
	stopDiff(port);

    sprintf(tempStr,"INTERFACEMODE %s UNICORE CMR ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"LOG %s CMROBS ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s CMRPLUS ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s CMRREF ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s CMRGLOOBS ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s CMRBD2OBS ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueSCMRX(unsigned char port, int id)
{
	return issueCMR(port,id);
}

bool cUnicoreCommand::issueDGPS(unsigned char port,double lat, double lon, float height, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
/*
INTERFACEMODE %s NOVATEL RTCM ON
LOG %s RTCM1 ONTIME 1
LOG %s RTCM31 ONTIME 1
LOG %s RTCM3 ONTIME 10
*/
	stopDiff(port);

    sprintf(tempStr,"INTERFACEMODE %s UNICORE RTCM ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"LOG %s RTCM1 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM31 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM3 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));
#ifdef ENABLE_SAVE
    sendcmd("saveconfig");
#endif
	return true;
}

bool cUnicoreCommand::issueBINEX(unsigned char port,int interval, int raw_eph)
{
	string str;
	char tempStr[128],intertemp[128];

	char portStr[32];
	if(port>10)
	{
		sprintf(portStr,"ICOM%d",port-10);
	}
	else
	{
		sprintf(portStr,"COM%d",port);
	}
	unlogall(port);
	sprintf(tempStr,"INTERFACEMODE %s UNICORE UNICORE on",portStr);//NONE
	str.assign(tempStr);
	if (!sendcmd(str));
	//return false;

	if(interval==0 && raw_eph==0)  return true;

	if(raw_eph > 0)
		sprintf(intertemp,"ONTIME %g",(float)raw_eph);
	else if(raw_eph < 0)
		strcpy(intertemp,"ONCHANGED");

	if(interval!=0){
		sprintf(tempStr,"LOG %s BINEX7F05 ONTIME %g",portStr,(float)interval/1000);
		str.assign(tempStr);
		if (!sendcmd(str))
			return false;
	}

	if(raw_eph != 0)
	{
		sprintf(tempStr,"LOG %s BINEX0101 %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));
		//return false;

		sprintf(tempStr,"LOG %s BINEX0102 %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));
		//return false;

		sprintf(tempStr,"LOG %s BINEX0105 %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));
		//return false;
	}

	sendcmd("saveconfig");

	return true;
}

bool cUnicoreCommand::issueRAWEPH(unsigned char port,int interval, int raw_eph)
{
	string str;
	char tempStr[128],intertemp[128];

	char portStr[32];
	if(port>10)
	{
		sprintf(portStr,"ICOM%d",port-10);
	}
	else
	{
		sprintf(portStr,"COM%d",port);
	}

	if(raw_eph > 0)
		sprintf(intertemp,"ONTIME %g",(float)raw_eph);
	else if(raw_eph < 0)
		strcpy(intertemp,"ONCHANGED");

	if(raw_eph != 0)
	{
		sprintf(tempStr,"LOG %s GPSEPHEMB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str))
			return false;

		sprintf(tempStr,"LOG %s IONUTCB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s GLOEPHEMERISB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s BD2EPHEMB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s BD2IONUTCB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s VERSION ONCE",portStr);
		str.assign(tempStr);
		if (!sendcmd(str));
	}

	return true;
}

bool cUnicoreCommand::issueRAW(unsigned char port,int interval, int raw_eph)
{
    string str;
    char tempStr[128],intertemp[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
    unlogall(port);
    sprintf(tempStr,"interfacemode %s UNICORE UNICORE on",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;
	syslog(LOG_LOCAL7|LOG_INFO,"interval:%ds, Raw_Eph: %ds",interval,raw_eph);
/*
LOG RANGEB ONTIME 1
LOG RAWEPHEMB ONCHANGED
LOG GLOEPHEMB ONCHANGED
LOG GLORAWEPHEMB ONCHANGED
LOG BDSEPHEMERISB ONCHANGED
LOG BDSRAWNAVSUBFRAMEB ONCHANGED
*/
    if(interval!=0)
    {
        sprintf(tempStr,"LOG %s RANGEB ONTIME %g",portStr,(float)interval/1000);
        str.assign(tempStr);
        if (!sendcmd(str))
            return false;

        sprintf(tempStr,"LOG %s TIMEB ONTIME %g",portStr,(float)interval/1000);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BESTPOSB ONTIME %g",portStr,(float)interval/1000);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BESTVELB ONTIME %g",portStr,(float)interval/1000);
        str.assign(tempStr);
        if (!sendcmd(str));
    }

	if(raw_eph > 0)
		sprintf(intertemp,"ONTIME %g",(float)raw_eph);
	else if(raw_eph < 0)
		strcpy(intertemp,"ONCHANGED");

	if(raw_eph != 0)
	{
		sprintf(tempStr,"LOG %s GPSEPHEMB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str))
			return false;

		sprintf(tempStr,"LOG %s IONUTCB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s GLOEPHEMERISB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s BD2EPHEMB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s BD2IONUTCB %s",portStr,intertemp);
		str.assign(tempStr);
		if (!sendcmd(str));

		sprintf(tempStr,"LOG %s VERSION ONCE",portStr);
		str.assign(tempStr);
		if (!sendcmd(str));
	}

    sendcmd("saveconfig");

	return true;
}

bool cUnicoreCommand::setElemask(unsigned char elem)
{
    string str;
    char tempStr[128];

    sprintf(tempStr,"ECUTOFF GPS %d.0",elem);
    str.assign(tempStr);
     if (!sendcmd(str))
		return false;

    sprintf(tempStr,"ECUTOFF GLONASS %d.0",elem);
    str.assign(tempStr);
     if (!sendcmd(str));

	sprintf(tempStr,"ECUTOFF  BD2 %d.0",elem);
    str.assign(tempStr);
     if (!sendcmd(str));

    return true;
}

bool cUnicoreCommand::issueRecord(int interval)
{
    string str;
    char tempStr[128];

	sprintf(tempStr,"LOG  RANGEB ONTIME %g",(float)interval/1000);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"LOG  RAWEPHEMB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;
/*
    sprintf(tempStr,"LOG  GLOEPHEMB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG  GLORAWEPHEMB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG  BDSEPHEMERISB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str));


    sprintf(tempStr,"LOG  BDSRAWNAVSUBFRAMEB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str));
*/
	return true;
}

bool cUnicoreCommand::stopRecord()
{
	//if (!sendcmd("unlog RANGEB")) return false;
	if (!sendcmd("unlog RAWEPHEMB")); return false;
	/*if (!sendcmd("unlog GLOEPHEMB"));
    if (!sendcmd("unlog GLORAWEPHEMB"));
    if (!sendcmd("unlog BDSEPHEMERISB"));
    if (!sendcmd("unlog BDSRAWNAVSUBFRAMEB"));
*/
	return true;
}

bool cUnicoreCommand::stopDiff(unsigned char port)
{
    string str;
    char tempStr[128];

    //sendcmd("fix none");
    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

	sprintf(tempStr,"unlogall %s",portStr);
	str.assign(tempStr);
	if (!sendcmd(str))
	{
		usleep(500*1000);
		if (!sendcmd(str))
		{
			usleep(500*1000);
			if (!sendcmd(str))
			{
				usleep(500*1000);
				if (!sendcmd(str))
				{
					return false;
				}
			}
		}
	}

    sprintf(tempStr,"INTERFACEMODE %s UNICORE UNICORE on",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));


	return true;
}

bool cUnicoreCommand::issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval)
{
    string str;
    char tempStr[128];
    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

  if(interval==0) trigger = TRIGGER_NONE;
  if(trigger == TRIGGER_NONE)
    sprintf(tempStr,"UNLOG %s %s\r\n",portStr,OEM_NMEA_MSG[msgtype]);
  else if(trigger == TRIGGER_ONTIME)
    sprintf(tempStr,"LOG %s %s ONTIME %g\r\n",portStr,OEM_NMEA_MSG[msgtype],((float)interval)/1000);
  else
    sprintf(tempStr,"LOG %s %s %s\r\n",portStr,OEM_NMEA_MSG[msgtype],OEM_NMEA_TRG[trigger]);

  //ctl_cout("Request nmea message %s",command);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    return true;
}

bool cUnicoreCommand::lognmea(unsigned char port, char *setting)
{
        static const char NMEA_MSG[][8] = {"GGA","GSA","GSV","ZDA","GST","GLL","RMC","VTG"};
        char *p1,*p2;
        char str[32];
        int  i,n,flag,end,interval;
        int msgnum=8;
        p1=(char *)setting;
        unlogall(port);
        for(i=0,end=0; i<msgnum && !end; i++)
        {
            p2=strchr(p1,'|');
            if(!p2)
            {
                p2=strchr(p1,'\0');
                end=1;
            }
            if(p2)
            {
                memset(str,0,sizeof(str));
                strncpy(str,p1,p2-p1);
                flag=0;
                for(n=0; n<msgnum && flag==0; n++)
                {
                    if(strstr(str,NMEA_MSG[n]))
                    {
                        flag=1;
                        break;
                    }
                }
                if(flag)
                {
                    p1=strchr(str,':');
                    if(p1)
                    {
                        p1+=1;
                        interval=atoi(p1);
                    }
                    else interval=0;
                    if(interval>0)
                    {
                    issuenmea(port,(OEM_NMEA_MSG_TYPE)n,TRIGGER_ONTIME,interval);
                    }
                }
                p1=p2+1;
            }
        }

    return true;
}

bool cUnicoreCommand::issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height, int id)
{
    bool res=true;
    unlogall(port);
    if(base_started==0) startbase(lat,lon,height,id);

    if(type==RTCM2)
    {
        res=issueRTCM23(port,id);
    }
    else if(type==CMR||type==SCMRX)
    {
         res=issueCMR(port,id);
    }
    else if(type==DGPS)
    {
        res=issueDGPS(port,lat, lon, height,id);
    }
    else if(type==RTCM32MSN)
    {
        res=issueRTCM32(port,id);
    }
	else if(type==RTCM3210)
	{
		res=issueRTCM3210(port,id);
	}
    else//RTCM3
    {
        res=issueRTCM30(port,id);
    }

    return res;
}

bool cUnicoreCommand::rtkmode(unsigned char port)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

    sprintf(tempStr,"INTERFACEMODE %s  AUTOMATIC  UNICORE OFF",portStr);//AUTO
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sprintf(tempStr,"unlogall %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;
/*
    sprintf(tempStr,"log %s gpgga ontime 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;
*/
    return true;
}


bool cUnicoreCommand::unlogall(unsigned char port)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

    sprintf(tempStr,"unlogall %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
	{
		usleep(500*1000);
		if (!sendcmd(str))
		{
			usleep(500*1000);
			if (!sendcmd(str))
			{
				usleep(500*1000);
				if (!sendcmd(str))
				{
					return false;
				}
			}
		}
	}

	sprintf(tempStr,"INTERFACEMODE %s UNICORE UNICORE on",portStr);
	str.assign(tempStr);
	if (!sendcmd(str));

    return true;
}


bool cUnicoreCommand::unlogallports()
{
    if (!sendcmd("unlogall"))
		return false;
	return true;
}

bool cUnicoreCommand::setDefault()
{
	base_started=0;
	if (!sendcmd("unlogall"))
		return false;
	if (!sendcmd("log  bestposb ontime 1"));
	if (!sendcmd("log  psrdopb ONCHANGED"));
	if (!sendcmd("log  timeb ontime 1"));

	if (!sendcmd("NETPORTCONFIG ICOM1 TCP 3001"));
	if (!sendcmd("NETPORTCONFIG ICOM2 TCP 3002"));
	if (!sendcmd("NETPORTCONFIG ICOM3 TCP 3003"));
	sendcmd("undulation egm96");

	sendcmd("fix none");
	sendcmd("interfacemode com2 UNICORE UNICORE on");
	sendcmd("interfacemode com1 UNICORE UNICORE on");
	sendcmd("NETCONFIG DHCP");
	sendcmd("dgpstimeout 60\r\nrtktimeout 20\r\n");

	sendcmd("nmeatalker auto");
	if (!sendcmd("saveconfig"))
		return false;

	return true;
}

bool cUnicoreCommand::setOemVersion(bool ver7)
{
	return true;
}

bool cUnicoreCommand::init()
{
    base_started=0;
    if (!sendcmd("log  versionb"))
    {
        sleep(1);
        if (!sendcmd("log  versionb"))
        {
            syslog(LOG_LOCAL7|LOG_ERR,"gps init failed.");
            return false;
        }
    }

    sendcmd("unlogall");
    if (!sendcmd("log  bestposb ontime 1"))
		return false;

    if (!sendcmd("log  psrdopb ONCHANGED"));
    if (!sendcmd("log  timeb ontime 1"))
		return false;

    if (!sendcmd("NETCONFIG DHCP"));
    if (!sendcmd("NETPORTCONFIG ICOM1 TCP 3001"));
    if (!sendcmd("NETPORTCONFIG ICOM2 TCP 3002"));
    if (!sendcmd("NETPORTCONFIG ICOM3 TCP 3003"));

	sendcmd("INTERFACEMODE ICOM1 UNICORE UNICORE on");
	sendcmd("INTERFACEMODE ICOM2 UNICORE UNICORE on");
	sendcmd("INTERFACEMODE ICOM3 UNICORE UNICORE on");

    if (!sendcmd("log  RANGEB ontime 1"));
    if (!sendcmd("log  SATVISB ontime 1"));
    if (!sendcmd("log  rtkdopb  ontime 1"));

	sendcmd("undulation egm96");
    sendcmd("fix none");

    //sendcmd("assignall sbas idle");
	usleep(200*1000);
    sendcmd("interfacemode com2 UNICORE UNICORE on");
    sendcmd("interfacemode com1 UNICORE UNICORE on");
    //sendcmd("interfacemode com3 UNICORE UNICORE on");
    sendcmd("log  versionb");
	sendcmd("saveconfig");
	return true;
}

bool cUnicoreCommand::setGlonass(int disable)
{
    string str;
    if(disable) str.assign("SYSCONTROL disable glonass");
    else str.assign("SYSCONTROL enable glonass");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cUnicoreCommand::setBeidou(int disable)
{
    string str;
    if(disable) str.assign("SYSCONTROL disable BD2");
    else str.assign("SYSCONTROL enable BD2");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cUnicoreCommand::setGPS(int disable)
{
    string str;
    //if(disable) str.assign("assignall GPS idle");
    //else str.assign("assignall GPS auto");
    if(disable) str.assign("SYSCONTROL disable GPS");
    else str.assign("SYSCONTROL enable GPS");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cUnicoreCommand::setGalileo(int disable)
{
    return true;
}

bool cUnicoreCommand::setSbas(int disable)
{

	return true;
}

bool cUnicoreCommand::setQzss(int disable)
{

	return true;
}

bool cUnicoreCommand::txtcommand(char *cmd)
{
    if (!sendcmd(cmd))
		return false;
    return true;
}
