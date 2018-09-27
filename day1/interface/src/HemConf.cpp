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
#include "HemConf.hpp"
#include "share.hpp"

using namespace std;

static char OEM_NMEA_MSG[][16] = {"GPGGA","GPGSA","GPGSV","GPZDA","GPGST","GPGLL","GPRMC","GPVTG","GPGGQ","GPGGK","GPGNS","LOGLISTA","COMCONFIGA","BESTPOSA"};

cHemisphereCommand::cHemisphereCommand() {
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


    base_started=0;
    error_count=0;
}

cHemisphereCommand::~cHemisphereCommand() {

	//lrd_test
	if(FreeSHM(&m_IPCGPSRAWWrite)!=0)
	{
		printf("cNovatelCommand: free fifo id:%d failed.\n",m_IPCGPSRAWWrite.iIPCMId);
	}

}

bool cHemisphereCommand::getResult() {
	return result;
}

void cHemisphereCommand::resetResult() {
	result = true;
}

bool cHemisphereCommand::sendcmd(string cmd) {
	//bool result = true;
	return sendOnce(cmd);
}

bool cHemisphereCommand::sendOnce(string cmd) {
	int i;
	int len;
	string command(cmd);
	command.append("\r\n");
	//syslog(LOG_LOCAL7|LOG_INFO,"oem : %s",cmd.c_str());
	len = strlen(command.c_str()) ;
	while(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0);//clear
    FIFO_Add_Buffer(&(m_IPCGPSRAWWrite.IPCFIFO),(unsigned char *)command.c_str(),len);
	for(i=0;i<15;i++)
	 {
        usleep(1000*100);//sleep(1);
        if(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0)
        {
            error_count=0;
            return true;
        }
	 }
    //printf("cmd timeout\n");
    syslog(LOG_LOCAL7|LOG_INFO,"oem cmd err: %s",cmd.c_str());
    error_count++;
    return false;
}


bool cHemisphereCommand::issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval)
{
    string str;
    char tempStr[128];
    char portStr[32];

    sprintf(portStr,"PORT%c",'A'-1+port);

  if(interval==0) trigger = TRIGGER_NONE;
  if(trigger == TRIGGER_NONE)
    sprintf(tempStr,"$JASC,%s,0,%s\r\n",OEM_NMEA_MSG[msgtype],portStr);
  else if(trigger == TRIGGER_ONTIME)
    sprintf(tempStr,"$JASC,%s,%.5f,%s\r\n",OEM_NMEA_MSG[msgtype],((float)1000/(float)interval),portStr);
  else
     sprintf(tempStr,"$JASC,%s,0,%s\r\n",OEM_NMEA_MSG[msgtype],portStr);

  //ctl_cout("Request nmea message %s",command);
    str.assign(tempStr);
    if (!sendcmd(str))
        return false;

    return true;
}

bool cHemisphereCommand::lognmea(unsigned char port, char *setting)
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

bool cHemisphereCommand::unlogall(unsigned char port)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    return true;
}

bool cHemisphereCommand::unlogallports()
{
    if (!sendcmd("$JOFF,PORTA"));
	if (!sendcmd("$JOFF,PORTB"));
	if (!sendcmd("$JOFF,PORTC"));
    return true;
}

bool cHemisphereCommand::rtkmode(unsigned char port)
{
    return unlogall(port);
}

bool cHemisphereCommand::issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height, int id)
{
    bool res=true;
    unlogall(port);
    if(base_started==0) startbase(lat,lon,height,id);

    if(type==RTCM2)
    {
        res=issueRTCM23(port, id);
    }
    else if(type==CMR||type==SCMRX)
    {
         res=issueCMR(port, id);
    }
    else if(type==DGPS)
    {
        res=issueDGPS(port,lat,lon,height, id);
    }
    else if(type==RTCM32MSN)
    {
        res=issueRTCM32(port, id);
    }
    else if(type==ROX)
    {
        res=issueROX(port, id);
    }
    else if(type==RTCM3210)
    {
        /**
        * NET20 PLUS定制差分输出
        */
		res=issueRTCM3210(port, id);
    }
    else//RTCM3
    {
        res=issueRTCM30(port, id);
    }

    return res;
}

bool cHemisphereCommand::issueRTCM23(unsigned char port, int id)
{
      string str;
     char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;


	sprintf(tempStr,"$JASC,RTCM,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueRTCM30(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;


	sprintf(tempStr,"$JASC,RTCM3,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    if (!sendcmd("$JRTCM3,EXCLUDE,MSM4"))
        return false;

#ifdef WD100 //航天九院定制
	if (!sendcmd("$JRTCM3,INCLUDE,4011,1019,1020,1046"))
    ;
#endif

    sendcmd("$JSAVE");
	return true;
}


bool cHemisphereCommand::issueROX(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;


	sprintf(tempStr,"$JASC,ROX,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueRTCMV3EPH(unsigned char port, string rtcmv3_eph)
{
    return true;
}

bool cHemisphereCommand::issueRTCM32(unsigned char port, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;


	sprintf(tempStr,"$JASC,RTCM3,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    if (!sendcmd("$JRTCM3,INCLUDE,MSM4"))
		;

    if (!sendcmd("$JRTCM3,INCLUDE,1019,1020,4011,1004,1012"))
        ;

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueRTCM3210(unsigned char port, int id)
{
	string str;
	char tempStr[128];

	char portStr[32];
	sprintf(portStr,"PORT%c",'A'-1+port);

	sprintf(tempStr,"$JOFF,%s",portStr);
	str.assign(tempStr);
	if (!sendcmd(str))
		return false;

	sprintf(tempStr,"$JASC,RTCM3,10,%s",portStr);
	str.assign(tempStr);
	if (!sendcmd(str))
	{
		sprintf(tempStr,"$JASC,RTCM3,5,%s",portStr);
		str.assign(tempStr);
		if (!sendcmd(str))
			return false;
	}

	if (!sendcmd("$JRTCM3,INCLUDE,1019,1020,4011"))
		;

	sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueCMR(unsigned char port, int id)
{
    string str;
    char tempStr[128];

        char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

	sprintf(tempStr,"$JASC,CMR,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueSCMRX(unsigned char port, int id)
{
    return issueCMR(port,id);
}



bool cHemisphereCommand::issueDGPS(unsigned char port,double lat, double lon, float height, int id)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sendcmd("$JMODE,BASE,YES");

    sprintf(tempStr,"$JRAD,1,%.9lf,%.9lf,%.3f",lat,lon,height);
    str.assign(tempStr);
    if (!sendcmd(str)) return false;

    sendcmd("$JRAD,9,1,1");

    sprintf(tempStr,"$JRAD,2");
    str.assign(tempStr);
    if (!sendcmd(str)){
        if (!sendcmd(str)){
            if (!sendcmd(str)){
                ;
            }
        }
    }

	sprintf(tempStr,"$JASC,RTCM,1,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("$JRAD,10,0");

    sendcmd("$JSAVE");

	return true;
}

bool cHemisphereCommand::issueBINEX(unsigned char port,int interval, int raw_eph)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    char intStr[32];
    sprintf(intStr,"%.5f",((float)1000/(float)interval));

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sprintf(tempStr,"$JBIN,95,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str))
        if(!sendcmd(str))
            return false;

    sprintf(tempStr,"$JBIN,94,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,35,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,36,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,65,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,66,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;
    //if(!sendcmd("$JBIN,75,"+str)) ;

    sprintf(tempStr,"$JBIN,76,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,80,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,62,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,30,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,98,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if(!sendcmd(str)) ;

    sprintf(tempStr,"$JBIN,45,%s,%s",intStr,portStr); //GALILEO ephemeris
    str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"$JBIN,16,%s,%s",intStr,portStr); //FULL_OBSERVATIONS
	str.assign(tempStr);
	if (!sendcmd(str));

    sendcmd("$JSAVE");

    return true;
}

bool cHemisphereCommand::issueRAWEPH(unsigned char port,int interval, int raw_eph)
{

    string str;
    char tempStr[128];

	char portStr[32]={0};
	if(port!=0)
		sprintf(portStr,",PORT%c",'A'-1+port);

    char intStr[32];
    sprintf(intStr,"%.5f",((float)1000/(float)interval));

    syslog(LOG_LOCAL7|LOG_INFO,"issueRAWEPH: %d\n", raw_eph);

    sprintf(tempStr,"$JBIN,35,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,65,%s,%s",intStr,portStr); //GLONASS ephemeris information
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,95,%s,%s",intStr,portStr); //GPS ephemeris information
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,45,%s,%s",intStr,portStr); //GALILEO ephemeris
    str.assign(tempStr);
    if (!sendcmd(str));

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::issueRAW(unsigned char port,int interval, int raw_eph)
{
    string str;
    char tempStr[128];

    char portStr[32];
    sprintf(portStr,"PORT%c",'A'-1+port);

    char intStr[32];
    sprintf(intStr,"%.5f",((float)1000/(float)interval));

    syslog(LOG_LOCAL7|LOG_INFO,"issueRAW: %d\n", raw_eph);

    sprintf(tempStr,"$JOFF,%s",portStr);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;

    sprintf(tempStr,"$JBIN,35,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,36,%s,%s",intStr,portStr);
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,65,%s,%s",intStr,portStr); //GLONASS ephemeris information
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,66,%s,%s",intStr,portStr); //GLONASS L1/L2 code and carrier phase information
    str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"$JBIN,76,%s,%s",intStr,portStr); //GPS L1/L2 code and carrier phase information
    str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"$JBIN,80,%s,%s",intStr,portStr); //SBAS data frame information
    str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"$JBIN,94,%s,%s",intStr,portStr); //Ionospheric and UTC conversion parameters
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,95,%s,%s",intStr,portStr); //GPS ephemeris information
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,62,%s,%s",intStr,portStr); //GLONASS almanac information
    str.assign(tempStr);
    if (!sendcmd(str));

    sprintf(tempStr,"$JBIN,45,%s,%s",intStr,portStr); //GALILEO ephemeris
    str.assign(tempStr);
    if (!sendcmd(str));

	sprintf(tempStr,"$JBIN,16,%s,%s",intStr,portStr); //FULL_OBSERVATIONS
	str.assign(tempStr);
	if (!sendcmd(str));

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::stopDiff(unsigned char port)
{
    return unlogall(port);
}

bool cHemisphereCommand::startbase(double lat, double lon, float height, int id)
{
    string str;
    char tempStr[128];

    //$JRTK,1,lat,lon,height
    sprintf(tempStr,"$JRTK,1,%.9lf,%.9lf,%.3f",lat,lon,height);
    str.assign(tempStr);
    //cout<< str<<endl;
     if (!sendcmd(str)) return false;

    sendcmd("$JMODE,BASE,YES\r\n");
    sendcmd("$JMODE,FIXLOC,YES\r\n");

    sprintf(tempStr,"$JRTK,28,%d",id);
    str.assign(tempStr);
    if (!sendcmd(str));

    base_started=1;

    return true;
}

bool cHemisphereCommand::stopbase()
{
    //if (!sendcmd("$JOFF,PORTB"))
     //   return false;
    base_started=0;
	return true;
}

bool cHemisphereCommand::checkbasestarted()
{

    if(base_started) return true;
    return false;
}

bool cHemisphereCommand::disableprn(int prn)
{
	string str;
	char tempStr[128];
	if(prn==0) return false;

	if(prn>200 && prn<=230) //Beidou
	{
		sprintf(tempStr,"$JPRN,EXCLUDE,BDS,%d\r\n",(prn-200));
	}
	else if(prn>64 && prn<=88) //GLONASS
	{
		sprintf(tempStr,"$JPRN,EXCLUDE,GLO,%d\r\n",(prn-64));
	}
	else if(prn>100 && prn<=136) //Galileo
	{
		sprintf(tempStr,"$JPRN,EXCLUDE,GAL,%d\r\n",(prn-100));
	}
	else  if(prn>0 && prn<=32)//GPS
	{
		sprintf(tempStr,"$JPRN,EXCLUDE,GPS,%d\r\n",prn);
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

bool cHemisphereCommand::enableallprn()
{
	if (!sendcmd("$JPRN,EXCLUDE,NONE\r\n"))
		return false;
	return true;
}

bool cHemisphereCommand::issueRecord(int interval)
{
     string str;
    char tempStr[128];
    sprintf(tempStr,"%.5f",(float)1000/interval);
    syslog(LOG_LOCAL7|LOG_INFO,"Record Interval: %s\n", tempStr);
    str.assign(tempStr);

//    if(!sendcmd("$JASC,GPGGA,"+str)) ;
//    sendcmd("$JSAVE");
//	return true;

    if(!sendcmd("$JBIN,95,"+str)) if(!sendcmd("$JBIN,95,"+str)) return false;
    if(!sendcmd("$JBIN,94,"+str)) ;

    if(!sendcmd("$JBIN,35,"+str)) ;
    if(!sendcmd("$JBIN,36,"+str)) ;
    if(!sendcmd("$JBIN,65,"+str)) ;
    if(!sendcmd("$JBIN,66,"+str)) ;
    //if(!sendcmd("$JBIN,75,"+str)) ;
    if(!sendcmd("$JBIN,76,"+str)) ;
    if(!sendcmd("$JBIN,80,"+str)) ;
    if(!sendcmd("$JBIN,62,"+str)) ;

    if(!sendcmd("$JBIN,30,"+str)) ;
	if(!sendcmd("$JBIN,16,"+str)) ;
    if(!sendcmd("$JBIN,98,"+str)) ;
    if(!sendcmd("$JBIN,45,"+str)) ;//GALILEO ephemeris
    //历书
#ifdef WD100 //航天九院定制
    if(!sendcmd("$JBIN,32,"+str)) ;
    if(!sendcmd("$JBIN,42,"+str)) ;
    if(!sendcmd("$JBIN,92,"+str)) ;
#endif
    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::stopRecord()
{
    sendcmd("$JBIN,35,0");
    sendcmd("$JBIN,36,0");
    sendcmd("$JBIN,45,0");
    sendcmd("$JBIN,65,0");
    sendcmd("$JBIN,66,0");
    //sendcmd("$JBIN,75,0");
    sendcmd("$JBIN,76,0");
    sendcmd("$JBIN,80,0");
    sendcmd("$JBIN,94,0");
    sendcmd("$JBIN,95,0");
    sendcmd("$JBIN,62,0");//GLONASS Almanac
	sendcmd("$JBIN,16,0");
    sendcmd("$JBIN,30,0");
    //历书
    /*sendcmd("$JBIN,32,0");//BeiDou Almanac
    sendcmd("$JBIN,42,0");//Galileo Almanac
    sendcmd("$JBIN,92,0");//GPS Almanac*/

    sendcmd("$JSAVE");
	return true;
}

bool cHemisphereCommand::txtcommand(char *cmd)
{
    if (!sendcmd(cmd))
		return false;

    return true;
}


bool cHemisphereCommand::setElemask(unsigned char elem)
{
    string str;
    char tempStr[128];

    sprintf(tempStr,"$JMASK,%d",elem);
    str.assign(tempStr);
    if (!sendcmd(str))
		return false;
    return true;
}


bool cHemisphereCommand::setGlonass(int disable)
{
    string str;
    if(disable) str.assign("$JMODE,GLOOFF,YES");
    else str.assign("$JMODE,GLOOFF,NO");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cHemisphereCommand::setBeidou(int disable)
{
    string str;
    if(disable) str.assign("$JMODE,BDSOFF,YES");
    else str.assign("$JMODE,BDSOFF,NO");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cHemisphereCommand::setGPS(int disable)
{
    string str;
    if(disable) str.assign("$JMODE,GPSOFF,YES");
    else str.assign("$JMODE,GPSOFF,NO");

    if (!sendcmd(str))
		return false;

    return true;
}

bool cHemisphereCommand::setGalileo(int disable)
{
	string str;
	if(disable) str.assign("$JSIGNAL,EXCLUDE,E1BC,E5B,E5A");//命令会自动保存
	else str.assign("$JSIGNAL,INCLUDE,E1BC,E5B,E5A");

	if (!sendcmd(str))
		return false;

	return true;
}

bool cHemisphereCommand::setSbas(int disable)
{
    string str;
    if(disable)
    {
        sendcmd("$JDIFF,EXCLUDE,SBAS");
        sendcmd("$JDIFF,EXCLUDE,EDIF");
    }
    else
    {
        sendcmd("$JDIFF,INCLUDE,SBAS");
        sendcmd("$JDIFF,INCLUDE,EDIF");
    }

    return true;
}

bool cHemisphereCommand::setQzss(int disable)
{
	return false;
}

bool cHemisphereCommand::setDefault()
{
    sendcmd("$JAGE,30");
    sendcmd("$JNMEA,GGAALLGNSS,YES");
    sendcmd("$JBAUD,115200,PORTB");
    sendcmd("$JBAUD,115200,PORTA");
    sendcmd("$JBAUD,115200,PORTC");
    sendcmd("$JSAVE");

	return true;
}

bool cHemisphereCommand::setOemVersion(bool ver7)
{
	return true;
}

bool cHemisphereCommand::init()
{

    if (!sendcmd("$JBIN,1,1"))
    {
        sleep (1);
        if (!sendcmd("$JBIN,1,1"))
            return false;
    }
    //if (!sendcmd("$JBIN,2,1")) return false;

    sendcmd("$JOFF");
    sendcmd("$JI");
    sendcmd("$JK,SHOW");
    if (!sendcmd("$JBIN,1,1"));
    if (!sendcmd("$JBIN,3,1"));
    if (!sendcmd("$JBIN,209,1"));

    //disable SBAS
    sendcmd("$JDIFF,EXCLUDE,SBAS");
    sendcmd("$JDIFF,EXCLUDE,EDIF");

    sendcmd("$JNMEA,GGAALLGNSS,YES");
    sendcmd("$JAGE,30");
    if (!sendcmd("$JBAUD,115200,PORTB"));
    if (!sendcmd("$JBAUD,115200,PORTC"));

	return true;
}
