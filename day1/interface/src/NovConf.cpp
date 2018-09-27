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
#include "NovConf.hpp"
#include "util.hpp"
#include "share.hpp"

using namespace std;

static char OEM_NMEA_TRG[][16] = {"ONTIME","ONCHANGED","ONCE"};
static char OEM_NMEA_MSG[][16] = {"GPGGALONG","GPGSA","GPGSV","GPZDA","GPGST","GPGLL","GPRMC","GPVTG","GPGGQ","GPGGK","GPGNS","LOGLISTA","COMCONFIGA","BESTPOSA"};

cNovatelCommand::cNovatelCommand()
{
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
    printf("cNovatelCommand: cmd fifo id:%d\n",key);
    base_started=0;
    error_count=0;
    oem7=0;
}

cNovatelCommand::~cNovatelCommand()
{

    //lrd_test
    if(FreeSHM(&m_IPCGPSRAWWrite)!=0)
    {
        printf("cNovatelCommand: free fifo id:%d failed\n",m_IPCGPSRAWWrite.iIPCMId);
    }

}

bool cNovatelCommand::getResult()
{
    return result;
}

void cNovatelCommand::resetResult()
{
    result = true;
}

bool cNovatelCommand::sendcmd(string cmd)
{
    //bool result = true;
    return sendOnce(cmd);
}

bool cNovatelCommand::sendOnce(string cmd)
{
    int i;
    int len;
    string command(cmd);
    command.append("\r\n");
    len = command.length() ;
    while(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0);//clear
    //syslog(LOG_LOCAL7|LOG_INFO,"oem cmd: %s",command.c_str());
    FIFO_Add_Buffer(&(m_IPCGPSRAWWrite.IPCFIFO),(unsigned char *)command.c_str(),len);
    for(i=0; i<10; i++)
    {
        usleep(1000*100);//sleep(1);
        if(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0)
        {
            error_count=0;
            return true;
        }
    }
    syslog(LOG_LOCAL7|LOG_INFO,"oem cmd err: %s",cmd.c_str());
    error_count++;
    return false;
}

bool cNovatelCommand::startbase(double lat, double lon, float height, int id)
{
    string str;
    char tempStr[128];

    sprintf(tempStr,"FIX POSITION %.9lf %.9lf %.3f",lat,lon,height);
    str.assign(tempStr);
    cout<< str<<endl;
    if (!sendcmd(str))
        return false;
    base_started=1;
    return true;
}

bool cNovatelCommand::stopbase()
{
    if (!sendcmd("fix none"))
        return false;
    base_started=0;
    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::checkbasestarted()
{
    if(base_started) return true;
    return false;
}

bool cNovatelCommand::disableprn(int prn)
{
    string str;
    char tempStr[128];
    if(prn==0) return false;

    if(prn>200 && prn<=230) //Beidou
    {
        sprintf(tempStr,"TRACKSV BEIDOU %d NEVER\r\n",(prn-200));
    }
    else if(prn>64 && prn<=88) //GLONASS
    {
        sprintf(tempStr,"TRACKSV GLONASS %d NEVER\r\n",(prn-64));
    }
    else if(prn>100 && prn<=136) //Galileo
    {
        sprintf(tempStr,"TRACKSV Galileo %d NEVER\r\n",(prn-100));
    }
    else  if(prn>0 && prn<=32)//GPS
    {
        sprintf(tempStr,"TRACKSV GPS %d NEVER\r\n",prn);
    }
    else
    {
        return false;
    }

    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    return true;
}


bool cNovatelCommand::enableallprn()
{
    //if (!sendcmd("unlockoutall"))
    if (!sendcmd("TRACKSV GPS 0 GOODHEALTH\r\nTRACKSV GLONASS 0 GOODHEALTH\r\nTRACKSV BEIDOU 0 GOODHEALTH\r\nTRACKSV Galileo 0 GOODHEALTH\r\n"))
        return false;

    return true;
}

bool cNovatelCommand::issueRTCM23(unsigned char port,int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL RTCM ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"GENERATERTKCORRECTIONS RTCM %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"DGPSTXID RTCM %d",id&1023);
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueRTCM30(unsigned char port,int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL RTCMV3 ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"GENERATERTKCORRECTIONS RTCMV3 %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"DGPSTXID RTCMV3 %d",id&4095);
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueRTCMV3EPH(unsigned char port,string rtcmv3_eph)
{
    string str;
    char tempStr[128];
    unsigned int split;

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

    while(1)
	{
	    if(rtcmv3_eph.empty())
        {
            break;
        }

        split = rtcmv3_eph.find('|',0);

		if(split != string::npos) //  找到|时
		{
		    str = rtcmv3_eph.substr(0,split);
			if(!str.empty())	//判断是否为空
			{
			    if(str=="BDS")
                {
                    sprintf(tempStr,"LOG %s RTCM1042 ONTIME 15",portStr);
                }
                else if(str=="GPS")
                {
                    sprintf(tempStr,"LOG %s RTCM1019 ONTIME 15",portStr);
                }
                else if(str=="GLN")
                {
                    sprintf(tempStr,"LOG %s RTCM1020 ONTIME 15",portStr);
                }
                else if(str=="QZSS")
                {
                    sprintf(tempStr,"LOG %s RTCM1044 ONTIME 15",portStr);
                }
                else if(str=="GAL")
                {
                    sprintf(tempStr,"LOG %s RTCM1045 ONTIME 15\r\nLOG %s RTCM1046 ONTIME 15\r\n",portStr,portStr);
                }

                str.assign(tempStr);
                if (!sendcmd(str));
			}
			rtcmv3_eph=rtcmv3_eph.substr(split+1);
		}
		else
        {
            break;
		}
	}

    return true;
}

bool cNovatelCommand::issueRTCM32(unsigned char port,int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL RTCMV3 ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;


    sprintf(tempStr,"LOG %s RTCM1074 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if (!sendcmd(str));
        //return false;
    }

    sprintf(tempStr,"LOG %s RTCM1084 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if (!sendcmd(str));
        //return false;
    }

    sprintf(tempStr,"LOG %s RTCM1005 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if (!sendcmd(str));
        //return false;
    }

    sprintf(tempStr,"LOG %s RTCM1008 ONTIME 15 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if (!sendcmd(str));
        //return false;
    }

    sprintf(tempStr,"LOG %s RTCM1124 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1094 ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1033 ONTIME 15 3",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"DGPSTXID RTCMV3 %d",id&4095);
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueRTCM3210(unsigned char port,int id)
{
    string str;
    char tempStr[128];
    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL RTCMV3 ON",portStr);
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

    sprintf(tempStr,"LOG %s RTCM1019 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1020 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RTCM1042 ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"DGPSTXID RTCMV3 %d",id&4095);
    str.assign(tempStr);
    if (!sendcmd(str));

    return true;
}

bool cNovatelCommand::issueROX(unsigned char port,int id)
{
    syslog(LOG_LOCAL7|LOG_ERR,"init rox failed.");
    return false;
}


bool cNovatelCommand::issueCMR(unsigned char port,int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL CMR ON",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"GENERATERTKCORRECTIONS CMR %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;
    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueSCMRX(unsigned char port,int id)
{
    return issueCMR(port,id);
}

bool cNovatelCommand::issueDGPS(unsigned char port,double lat, double lon, float height,int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"INTERFACEMODE %s NOVATEL RTCM ON",portStr);
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

    sprintf(tempStr,"DGPSTXID CMR %d",id&31);
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueBINEX(unsigned char port,int interval, int raw_eph)
{
    issueRAW(port,interval,raw_eph);

    return true;
}

bool cNovatelCommand::issueRAWEPH(unsigned char port,int interval, int raw_eph)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
#if 1
    if(raw_eph==0)
        ;
    else if(raw_eph<=-1)
    {
        sprintf(tempStr,"LOG %s RAWEPHEMB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLORAWEPHEMB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));
    }
    else
    {
        sprintf(tempStr,"LOG %s RAWEPHEMB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLORAWEPHEMB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

//===================================================================
        sprintf(tempStr,"LOG %s RAWEPHEMB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str))
            return false;

        sprintf(tempStr,"LOG %s GLORAWEPHEMB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));
    }
#else
    sprintf(tempStr,"LOG %s PASSTHROUGHB ONCHANGED",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s ALMANACB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s GLOALMANACB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s BDSALMANACB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s GPSEPHEMB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s GLOEPHEMERISB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s BDSEPHEMRISB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s IONUTCB ONNEW",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RXSTATUSB ONCHANGED",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RXCONFIG",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s VERSION",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

#endif
    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::issueRAW(unsigned char port,int interval, int raw_eph)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
    unlogall(port);
    sprintf(tempStr,"interfacemode %s novatel novatel on",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));
    {
        if (!sendcmd(str));
        //return false;
    }

#if 1
    syslog(LOG_LOCAL7|LOG_INFO,"interval:%ds, Raw_Eph: %ds",interval,raw_eph);
    /*
    LOG RANGEB ONTIME 1
    LOG RAWEPHEMB ONCHANGED
    LOG GLOEPHEMB ONCHANGED
    LOG GLORAWEPHEMB ONCHANGED
    LOG BDSEPHEMERISB ONCHANGED
    LOG BDSRAWNAVSUBFRAMEB ONCHANGED
    LOG GALEPHEMERISB ONCHANGED
    */
    if(interval!=0)
    {
        sprintf(tempStr,"LOG %s RANGEB ONTIME %.3f",portStr,(float)interval/1000);
        str.assign(tempStr);
        if (!sendcmd(str))
        {
            if (!sendcmd(str));
            return false;
        }
    }

    if(raw_eph==0)
        ;
    else if(raw_eph<=-1)
    {
        sprintf(tempStr,"LOG %s RAWEPHEMB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str))
        {
            if (!sendcmd(str));
            //return false;
        }

#if 0
        sprintf(tempStr,"LOG %s GLOEPHEMB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));
#endif
        sprintf(tempStr,"LOG %s GLORAWEPHEMB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB ONCHANGED",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));
    }
    else
    {
        sprintf(tempStr,"LOG %s RAWEPHEMB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));
        {
            if (!sendcmd(str));
            //return false;
        }

        sprintf(tempStr,"LOG %s GLORAWEPHEMB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB",portStr);
        str.assign(tempStr);
        if (!sendcmd(str));

//===================================================================
        sprintf(tempStr,"LOG %s RAWEPHEMB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str))
            return false;

        sprintf(tempStr,"LOG %s GLORAWEPHEMB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GLOEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s BDSRAWNAVSUBFRAMEB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));

        sprintf(tempStr,"LOG %s GALEPHEMERISB ONTIME %.3f",portStr, (float)raw_eph);
        str.assign(tempStr);
        if (!sendcmd(str));
    }

    sprintf(tempStr,"LOG %s IONUTCB ONCHANGED",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));
#else
    sprintf(tempStr,"LOG %s TRACKSTATB ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s FRONTENDDATAB ONTIME 1",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s CLOCKSTEERINGB ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s SATVIS2B ONTIME 60",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RANGEB ONTIME 0.5",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s BESTPOSB ONTIME 10",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s RXCONFIG",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"LOG %s VERSION",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

#endif
    sendcmd("SAVECONFIG");
    return true;
}

bool cNovatelCommand::setElemask(unsigned char elem)
{
    string str;
    char tempStr[128];

    //sprintf(tempStr,"ECUTOFF %d\r\n",elem);ELEVATIONCUTOFF ALL
    sprintf(tempStr,"ELEVATIONCUTOFF ALL %d\r\n",elem);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    return true;
}

bool cNovatelCommand::issueRecord(int interval)
{
    string str;
    char tempStr[128];

    sprintf(tempStr,"LOG  RANGEB ONTIME %.3f",(float)interval/1000);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    sprintf(tempStr,"LOG  RAWEPHEMB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

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

    sprintf(tempStr,"LOG IONUTCB ONCHANGED");
    str.assign(tempStr);
    if (!sendcmd(str));

    return true;
}

bool cNovatelCommand::stopRecord()
{
    if (!sendcmd("unlog RANGEB"))
        return false;
    if (!sendcmd("unlog RAWEPHEMB"));
    if (!sendcmd("unlog GLOEPHEMB"));
    if (!sendcmd("unlog GLORAWEPHEMB"));
    if (!sendcmd("unlog BDSEPHEMERISB"));
    if (!sendcmd("unlog BDSRAWNAVSUBFRAMEB"));

    return true;
}

bool cNovatelCommand::stopDiff(unsigned char port)
{
    string str;
    char tempStr[128];

    //sendcmd("fix none");
    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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

    sprintf(tempStr,"interfacemode %s novatel novatel on",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if (!sendcmd(str));
        //return false;
    }


    return true;
}

bool cNovatelCommand::issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval)
{
    string str;
    char tempStr[128];
    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
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
        sprintf(tempStr,"LOG %s %s ONTIME %0.3f\r\n",portStr,OEM_NMEA_MSG[msgtype],((float)interval)/1000);
    else
        sprintf(tempStr,"LOG %s %s %s\r\n",portStr,OEM_NMEA_MSG[msgtype],OEM_NMEA_TRG[trigger]);

    //ctl_cout("Request nmea message %s",command);
    str.assign(tempStr);
    if (!sendcmd(str))
    {
        if(msgtype==GGA)
        {
            str=string_replace(str,"GPGGALONG","GPGGA");
            if (!sendcmd(str)) return false;
        }
        else  return false;
    }

    return true;
}

bool cNovatelCommand::lognmea(unsigned char port, char *setting)
{
    stopDiff(port);

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

bool cNovatelCommand::issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height, int id)
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
        res=issueDGPS(port,lat,lon,height,id);
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

bool cNovatelCommand::rtkmode(unsigned char port)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }

    sprintf(tempStr,"INTERFACEMODE %s AUTO NOVATEL OFF",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;
    /*
        sprintf(tempStr,"unlogall %s",portStr);
        str.assign(tempStr);
        if (!sendcmd(str))
    		return false;

        sprintf(tempStr,"log %s gpgga ontime 10",portStr);
        str.assign(tempStr);
        if (!sendcmd(str))
    		return false;
    */
    return true;
}


bool cNovatelCommand::unlogall(unsigned char port)
{
    string str;
    char tempStr[128];

    char portStr[32];
    if(port>20)
    {
        sprintf(portStr,"USB%d",port-20);
    }
    else if(port>10)
    {
        sprintf(portStr,"ICOM%d",port-10);
    }
    else
    {
        sprintf(portStr,"COM%d",port);
    }
//syslog(LOG_LOCAL7|LOG_INFO,"=======:  %s\n",portStr);
    sprintf(tempStr,"unlogall %s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    return true;
}


bool cNovatelCommand::unlogallports()
{
    if (!sendcmd("unlogall"))
        return false;
    return true;
}

bool cNovatelCommand::setDefault()
{
    base_started=0;
    if (!sendcmd("unlogall"))
        return false;
    if (!sendcmd("log  bestposb ontime 1"))
        return false;
    if (!sendcmd("log  psrdopb ontime 1"))
        return false;
    if (!sendcmd("log  timeb ontime 1"))
        return false;

    sendcmd("fix none");
    sendcmd("interfacemode com2 novatel novatel on");
    sendcmd("interfacemode com1 novatel novatel on");
    sendcmd("ipconfig etha dhcp");
    sendcmd("ethconfig etha auto auto auto normal");
    sendcmd("dgpstimeout 60\r\nrtktimeout 20\r\n");
    sendcmd("SERIALCONFIG com3 115200 n 8 1 n off");
    sendcmd("SERIALCONFIG com1 115200 n 8 1 n off");
    sendcmd("SERIALCONFIG com2 115200 n 8 1 n off");

    sendcmd("nmeatalker auto");
    if (!sendcmd("saveconfig"))
        return false;

    return true;
}

bool cNovatelCommand::setOemVersion(bool ver7)
{
    oem7=ver7;

    return true;
}

bool cNovatelCommand::init()
{
    base_started=0;
    if (!sendcmd("log  versionb"))
    {
        sleep(1);
        if (!sendcmd("log  versionb"))
        {
            sleep(1);
            if (!sendcmd("log  versionb"))
            {
                syslog(LOG_LOCAL7|LOG_ERR,"gps init failed.");
                return false;
            }
        }
    }

    sendcmd("unlogall");
    if (!sendcmd("log  bestposb ontime 1"))
        return false;

    if (!sendcmd("log  psrdopb ontime 1"))
        return false;
    if (!sendcmd("log  timeb ontime 1"))
        return false;

    if (!sendcmd("log  RANGECMPB ontime 1"));
    if (!sendcmd("log  SATVIS2B ontime 1"));
    if (!sendcmd("log  BESTSATSB  ontime 1"));
    sendcmd("fix none");
    sendcmd("UNDULATION USER 0.000000");
    sendcmd("assignall sbas idle");
    sendcmd("ipconfig etha dhcp");
    sendcmd("SERIALCONFIG com2 115200 n 8 1 n off");
    sendcmd("SERIALCONFIG com1 115200 n 8 1 n off");
    sendcmd("SERIALCONFIG com3 115200 n 8 1 n off");
    usleep(200*1000);
    sendcmd("interfacemode com2 novatel novatel on");
    sendcmd("interfacemode com1 novatel novatel on");
    //sendcmd("interfacemode com3 novatel novatel on");

    if (!sendcmd("log  versionb"));
    return true;
}

bool cNovatelCommand::setGlonass(int disable)
{
    string str;
    if(disable) str.assign("assignall glonass idle");
    else str.assign("assignall glonass auto");

    if (!sendcmd(str))
        return false;

    return true;
}

bool cNovatelCommand::setBeidou(int disable)
{
    string str;
    if(disable)
    {
        str.assign("assignall beidou idle");
        if (!sendcmd(str))
            return false;
    }
    else
    {
        str.assign("assignall beidou auto");
        if (!sendcmd(str))
            return false;
        if(oem7)sendcmd("SELECTCHANCONFIG 4");
        else sendcmd("SELECTCHANCONFIG 2");
    }

    return true;
}

bool cNovatelCommand::setGPS(int disable)
{
    string str;
    if(disable) str.assign("assignall gps idle");
    else str.assign("assignall gps auto");

    if (!sendcmd(str))
        return false;

    return true;
}

bool cNovatelCommand::setGalileo(int disable)
{
    string str;
    if(disable)
    {
        str.assign("assignall galileo idle");
        if (!sendcmd(str))
            return false;
    }
    else
    {
        str.assign("assignall galileo auto");
        if (!sendcmd(str))
            return false;
        sendcmd("UNLOCKOUTSYSTEM GALILEO");
    }

    return true;
}

bool cNovatelCommand::setSbas(int disable)
{
    string str;

    if(disable)
    {
        str.assign("sbascontrol disable\r\nassignall sbas idle\r\n");
        if (!sendcmd(str))return false;
    }
    else
    {
        str.assign("sbascontrol enable\r\nassignall sbas auto\r\n");
        if (!sendcmd(str))return false;
        sendcmd("UNLOCKOUTSYSTEM SBAS");
    }
    return true;
}

bool cNovatelCommand::setQzss(int disable)
{
    string str;
    if(disable) str.assign("assignall qzss idle");
    else str.assign("assignall qzss auto");

    if (!sendcmd(str))
        return false;

    return true;
}

bool cNovatelCommand::txtcommand(char *cmd)
{
    if (!sendcmd(cmd))
        return false;
    return true;
}
