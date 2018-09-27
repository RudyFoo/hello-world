/*
 * NovConf.cpp
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
#include "../curl/curl.h"
#include "../curl/types.h"
#include "../curl/easy.h"

#include "TrimbleConf.hpp"
#include "share.hpp"

using namespace std;

cTrimbleCommand::cTrimbleCommand() {
	if ((cmdmailbox = msgget(KEY_CMDMAILBOX, IPC_CREAT |0666)) == -1)
    {
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

    error_count=0;
    base_started=0;

	memset(gpsdisabletab,0,sizeof(gpsdisabletab));
	memset(glonassdisabletab,0,sizeof(glonassdisabletab));
	memset(beidoudisabletab,0,sizeof(beidoudisabletab));
}

cTrimbleCommand::~cTrimbleCommand() {
	//lrd_test
	if(FreeSHM(&m_IPCGPSRAWWrite)!=0)
	{
		printf("cTrimbleCommand: free fifo id:%d failed\n",m_IPCGPSRAWWrite.iIPCMId);
	}
}

bool cTrimbleCommand::getResult() {
	return result;
}

void cTrimbleCommand::resetResult() {
	result = true;
}

bool cTrimbleCommand::send(string cmd) {
	//bool result = true;
    return true;
}

//向指定端口发送主板命令
void cTrimbleCommand::sendcommand()
{
  int length = 0;

  while(length != command.length)
  {
	length  += FIFO_Add_Buffer(&(m_IPCGPSRAWWrite.IPCFIFO),&command.data[length],command.length - length);
  }


}

void cTrimbleCommand::countsum()
{
  unsigned char sum = 0;
  for(int i = 1; i < command.length; i++)
    sum += command.data[i];
  command.data[command.length] = sum;
  command.length += 1;
}

void cTrimbleCommand::swap8(void * ptr)
{
  unsigned char *bptr, bTemp[8];

  bptr = (unsigned char *) ptr;
  for(int i=0; i<8; i++)
    bTemp[7-i] = *bptr++;

  bptr = (unsigned char *) ptr;
  for(int i=0; i<8; i++)
    *bptr++ = bTemp[i];
}


void cTrimbleCommand::swap4(void * ptr)
{
  unsigned char *bptr, bTemp[4];

  bptr = (unsigned char *) ptr;
  for(int i=0; i<4; i++)
    bTemp[3-i] = *bptr++;

  bptr = (unsigned char *) ptr;
  for(int i=0; i<4; i++)
    *bptr++ = bTemp[i];
}

void cTrimbleCommand::swap2(void * ashort)
{
  unsigned char * bptr = (unsigned char *)ashort;
  unsigned char tmp = *bptr;
  *bptr = *(bptr+1);
  *(bptr+1) = tmp;
}

void cTrimbleCommand::commandgetserial()
{
    command.data[0] = STX;
    command.data[1] = 0x00;
    command.data[2] = 0x06;
    command.data[3] = 0x00;
    command.length = 4;
    countsum();
    command.data[command.length] = ETX;
    command.length += 1;
}

void cTrimbleCommand::clearappfilerecords()
{
  appbuf.data[0] = 0x03;
  appbuf.data[1] = 0x00; //all device
  appbuf.data[2] = 0x01; //apply appfile immediately
  appbuf.data[3] = 0x00; //Alter receiver parameters only as specified in the application file. Leave unspecified settings alone
  appbuf.length = 4;
  appbuf.index = 0;
}

//停掉某个端口的消息输出
bool cTrimbleCommand::applystopallrecord(unsigned char port)
{
  int recordlength = 7;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x05;				//length
  recordBuf[2] = 0xFF;		                //OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = 00;			        //Frequency
  recordBuf[5] = 00;				//offset
  recordBuf[6] = 00;				//message flag
  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

//设置elevMask(取值范围0x00-0x5A)，pdopmask(取值范围0x00-0xff)
bool cTrimbleCommand::applygeneralcontrolsrecord(unsigned char elevMask, unsigned char pdopMask)
{
  int recordlength = 10;
  unsigned char recordBuf[256];
  recordBuf[0] = 0x01;					//Record type
  recordBuf[1] = 0x08;					//length
  recordBuf[2] = elevMask;				//elevation mask
  recordBuf[3] = 0x00;					//measurement rate   1HZ
  recordBuf[4] = pdopMask;				//pdop mask
  recordBuf[5] = 0x00;						//reserved
  recordBuf[6] = 0x00;						//reserved
  recordBuf[7] = 0x01;			//RTK positioning mode,0:Synchronous positioning
  recordBuf[8] = 0x00;	//Positioning solution selection,0:Use best available solution
  recordBuf[9] = 0xff;						//reserved

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

bool cTrimbleCommand::applyrt17record(unsigned char port, unsigned char frequency)
{
  int recordlength = 10;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x08;				//length
  recordBuf[2] = 0x04;		                //OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = frequency;			//Frequency
  recordBuf[5] = 0x00;				//offset
  recordBuf[6] = 0x0F;				//RT17 FLAG 0x0e (0x0F-Concise record format)
  recordBuf[7] = 0x11;				//RT17 FLAG2
  recordBuf[8] = 0x00;				//reserved
  recordBuf[9] = 0x00;				//reserved

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}



bool cTrimbleCommand::applybinexrecord(unsigned char port, unsigned char frequency)
{
  int recordlength = 10;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x08;				//length
  recordBuf[2] = 39;		                //OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = frequency;			//Frequency
  recordBuf[5] = 0x00;				//offset
  recordBuf[6] = 0x0F;				//RT17 FLAG 0x0e (0x0F-Concise record format)
  recordBuf[7] = 0x11;				//RT17 FLAG2
  recordBuf[8] = 0x00;				//reserved
  recordBuf[9] = 0x00;				//reserved

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

bool cTrimbleCommand::commandappfile()
{
  int pagecount = (appbuf.length / 245) + 1;
  int length;
  bool lastpage = false;
  if(appbuf.index * 245  < appbuf.length )
  {
    if(((appbuf.index + 1) * 245) > appbuf.length)
    {
      length = appbuf.length - appbuf.index * 245;
      lastpage = true;
    }
    else
      length = 245;
    command.data[0] = STX;
    command.data[1] = 0x00;
    command.data[2] = 0x64;
    command.data[3] = length+3;
    command.data[4] = 0x00;
    command.data[5] = appbuf.index;
    command.data[6] = pagecount - 1;
    memcpy(&(command.data[7]), &(appbuf.data[appbuf.index * 245]), length);
    command.length = length + 7;
    countsum();
    command.data[command.length] = ETX;
    command.length += 1;
    appbuf.index++;
  }
  return lastpage;
}


//name:reference node description,rtcmid:station id,cmrid:rtk station
bool cTrimbleCommand::applyreferencenoderecord(char* name, double lat, double lon, double height, int rtcmid, int cmrid)
{
  int recordlength = 91;
  unsigned char  recordBuf[256];
  memset(recordBuf,0,sizeof(recordBuf));
  recordBuf[0] = 0x03;				//Record type
  recordBuf[1] = 89;				//length
  recordBuf[2] = 0x00;				//FLAG reserved
  recordBuf[3] = 0x00;				//NODE INDEX reserved
  memcpy(&(recordBuf[4]), (void*)name, 8);			//8-character reference node description
  memcpy(&(recordBuf[12]), &lat, 8);			//Lat
  memcpy(&(recordBuf[20]), &lon, 8);			//Lon
  memcpy(&(recordBuf[28]), &height, 8);			//altitude
  swap8(&(recordBuf[12]));			//Lat
  swap8(&(recordBuf[20]));			//Lon
  swap8(&(recordBuf[28]));			//altitude
  memcpy(&(recordBuf[36]), &rtcmid, 2);			//
  swap2(&(recordBuf[36]));			//
  recordBuf[38] = cmrid;				//CMR id
  recordBuf[39]=0x40;
  recordBuf[40]=0x41;
  memcpy(&(recordBuf[41]), (void*)name, 8);	//STATION NAME
  recordBuf[61]=0x42;
  memcpy(&(recordBuf[62]), (void*)name, 8);	//STATION CODE
  memset(&recordBuf[78],0x20,11);
  memcpy(&recordBuf[89], &rtcmid, 2); //RTCM3 Station ID
  swap2(&recordBuf[89]);			//

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

bool cTrimbleCommand::applyrtcmrecord(unsigned char port, unsigned char frequency, unsigned char offset, unsigned char rtcmflag, unsigned char rtcmflag2)
{
  int recordlength = 10;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x08;				//length
  recordBuf[2] = 0x03;		//OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = frequency;			//Frequency
  recordBuf[5] = offset;				//offset
  recordBuf[6] = rtcmflag;				//GSOF subtype
  recordBuf[7] = rtcmflag2;
  recordBuf[8] = 0x00;				//reserved
  recordBuf[9] = 0x00;				//reserved

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

bool cTrimbleCommand::applycmrrecord(unsigned char port,unsigned char frequency, unsigned char offset, unsigned char msgtype)
{
  int recordlength = 10;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x08;				//length
  recordBuf[2] = 0x02;		//OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = frequency;			//Frequency
  recordBuf[5] = offset;				//offset
  recordBuf[6] = msgtype;				//GSOF subtype
  recordBuf[7] = 0;
  recordBuf[8] = 0;
  recordBuf[9] = 0;
  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

//申请GGA
bool cTrimbleCommand::applynmeav41(int v41,char port, char frequency, char offset, char type)
{
	int recordlength = 13;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x07;				//Record type
	recordBuf[1] = 11;				//length
	recordBuf[2] = type;		                //OUTPUT MESSAGE TYPE
	recordBuf[3] = port;				//Port index
	recordBuf[4] = frequency;			//Frequency
	recordBuf[5] = offset;			//offset
	if(type==0x26&&v41>0)recordBuf[6]=0xC8;
	else recordBuf[6]=0x48;

	if(type==0x06)recordBuf[7]=1;
	else recordBuf[7]=1-v41;
	//flag2 //0-GNGGA  1-GPGGA
	recordBuf[8] = 0x0;				//BANDWIDTH LIMIT
	recordBuf[9] = 0x0;
	recordBuf[10] = 0x0;
	recordBuf[11] = 0x0;

	recordBuf[12] = 0x0;

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}

}

//申请nmea消息，type:0x06-0x08,0x0c-0x12
bool cTrimbleCommand::applynmearecord(unsigned char port, unsigned char frequency, unsigned char offset, unsigned char type)
{
  int recordlength = 7;
  unsigned char  recordBuf[256];
  if((type==0x06||type==0x26)&&frequency>0)//GGA
  {
	  return applynmeav41(1,port,frequency,offset,type);
  }

  recordBuf[0] = 0x07;				//Record type
  recordBuf[1] = 0x05;				//length
  recordBuf[2] = type;		                //OUTPUT MESSAGE TYPE
  recordBuf[3] = port;				//Port index
  recordBuf[4] = frequency;			//Frequency
  recordBuf[5] = offset;			//offset
  recordBuf[6] = 0x00;				//flag

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}


//开启/禁用SBAS
bool cTrimbleCommand::applygsbasrecord(int enable)
{
  int recordlength = 26;
  unsigned char  recordBuf[256];
  unsigned char sbas[24]={0x00,0x00,0x00,0x04,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x03};

  recordBuf[0] = 0x16;				//Record type
  recordBuf[1] = 0x18;				//length
  //recordBuf[2] = 0x00;				//FLAG reserved

  if(enable) sbas[0]=5;//3-SBAS	L1CA               5-SBAS	L1CA+L5
  memcpy(&recordBuf[2],sbas,sizeof(sbas));


  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}


bool cTrimbleCommand::applyqzssrecord(int disable)
{
	int i;
	int recordlength = 7;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x5F;				//Record type
	recordBuf[1] = 5;				//length
	//recordBuf[2] = 0x00;				//FLAG reserved

	for(i=0;i<5;i++)
	{
		recordBuf[2+i]=disable;
	}

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}
}

//开启/禁用Glonass
bool cTrimbleCommand::applyglonassrecord(int disable)
{
  int i;
  int recordlength = 26;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x37;				//Record type
  recordBuf[1] = 0x18;				//length
  //recordBuf[2] = 0x00;				//FLAG reserved

  for(i=0;i<24;i++)
  {
    recordBuf[2+i]=disable;
  }

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}

//开启/禁用Beidou
bool cTrimbleCommand::applybeidourecord(int disable)
{
  int i;
  int recordlength = 32;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x50;				//Record type
  recordBuf[1] = 0x1E;				//length
  //recordBuf[2] = 0x00;				//FLAG reserved

  for(i=0;i<30;i++)
  {
    recordBuf[2+i]=disable;
    //glonassdisabletab[i]=disable;
  }

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}


//开启/禁用Galileo
bool cTrimbleCommand::applygalileorecord(int disable)
{
  int i;
  int recordlength = 54;
  unsigned char  recordBuf[256];

  recordBuf[0] = 0x4F;				//Record type
  recordBuf[1] = 0x34;				//length
  //recordBuf[2] = 0x00;				//FLAG reserved

  for(i=0;i<52;i++)
  {
    recordBuf[2+i]=disable;
    //glonassdisabletab[i]=disable;
  }

  if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
    return false;
  else
  {
    memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
    appbuf.length += recordlength;
    return true;
  }
}


//开启/禁用GPS
bool cTrimbleCommand::applygpsrecord(int disable)
{
    int i;
    int recordlength = 58;
    unsigned char  recordBuf[256];

    recordBuf[0] = 0x06;				//Record type
    recordBuf[1] = 0x38;				//length

    for(i=0;i<56;i++)
    {
        recordBuf[2+i]=disable;
    }

    if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
        return false;
    else
    {
        memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
        appbuf.length += recordlength;
        return true;
    }
}


//禁用GPS(0-31)
bool cTrimbleCommand::disablegpssnr(int snr)
{
	int i;
	int recordlength = 58;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x06;				//Record type
	recordBuf[1] = 0x38;				//length

	for(i=0;i<56;i++)
	{
		recordBuf[2+i]=0;
	}

	gpsdisabletab[snr]=1;
	for(i=0;i<32;i++)
	{
		recordBuf[2+i]=gpsdisabletab[i];
	}

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}
}

//禁用Glonass(0-31)
bool cTrimbleCommand::disableglonasssnr(int snr)
{
	int i;
	int recordlength = 26;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x37;				//Record type
	recordBuf[1] = 0x18;				//length
	//recordBuf[2] = 0x00;				//FLAG reserved

	glonassdisabletab[snr]=1;
	for(i=0;i<24;i++)
	{
		recordBuf[2+i]=glonassdisabletab[i];
	}

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}
}
//禁用beidou(0-31)
bool cTrimbleCommand::disablebeidousnr(int snr)
{
	int i;
	int recordlength = 32;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x50;				//Record type
	recordBuf[1] = 0x1e;				//length
	//recordBuf[2] = 0x00;				//FLAG reserved

	beidoudisabletab[snr]=1;
	for(i=0;i<30;i++)
	{
		recordBuf[2+i]=beidoudisabletab[i];
	}

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}
}
//禁用Galileo(0-35)
bool cTrimbleCommand::disablegalileosnr(int snr)
{
	int i;
	int recordlength = 54;
	unsigned char  recordBuf[256];

	recordBuf[0] = 0x4F;				//Record type
	recordBuf[1] = 0x34;				//length
	//recordBuf[2] = 0x00;				//FLAG reserved

	galileodisabletab[snr]=1;
	for(i=0;i<36;i++)
	{
		recordBuf[2+i]=galileodisabletab[i];
		//glonassdisabletab[i]=disable;
	}

	if (appbuf.length + recordlength > OEM_APPFILE_BUFFER_LENGTH)
		return false;
	else
	{
		memcpy(&(appbuf.data[appbuf.length]), recordBuf, recordlength);
		appbuf.length += recordlength;
		return true;
	}
}

#define MAX_RETRY 3
bool cTrimbleCommand::sendbincommand(bool isappfile)
{
  int i=0,n;
  int succeed;
  if(isappfile)
  {
    while(!commandappfile()&& i++<5)
    {
      for(n=0;n<MAX_RETRY;n++)
      {
		succeed=0;
		while(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0);//clear
        sendcommand();
		for(i=0;i<10;i++)
		{
			usleep(1000*100);//sleep(1);
			if(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0)
			{
				succeed=1;
				break;
			}
	    }
        if(n>1) sleep(1);
      }
    }
  }

  for(n=0;n<MAX_RETRY;n++)
  {
	succeed=0;
	while(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0);//clear
    sendcommand();
	for(i=0;i<10;i++)
	{
		usleep(1000*100);//sleep(1);
		if(msgrcv(cmdmailbox, &mail, 8, 1, IPC_NOWAIT)>=0)
		{
			succeed=1;
			break;
		}
	}
	if(!succeed)
	{
		if(n>1) sleep(1);
		continue;
	}
    if(isappfile)
    {
      appbuf.length = 0;
      appbuf.index = 0;
    }
	error_count=0;
    return true;
  }
  error_count++;
  return false;
}

/***********************************************************************************/
bool cTrimbleCommand::unlogall(unsigned char port)
{
    clearappfilerecords();
    applystopallrecord(port);
    if(!sendbincommand(true))
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"unlogall %d failed.",port);
		return false;
	}
    return true;
}

bool cTrimbleCommand::unlogallports()
{
    int i,n;
    unsigned char ports[]={20,21,22,23,24,25,26,27,28,29};
    n=10;
    clearappfilerecords();
    for(i=0;i<n;i++) applystopallrecord(ports[i]);
    if(!sendbincommand(true))
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"unlogallports failed.");
		return false;
	}
    return true;
}

bool cTrimbleCommand::rtkmode(unsigned char port)
{
     //return unlogall(port);
     //return issuenmea(port,GGA,TRIGGER_ONTIME,10000);
     return true;
}

bool cTrimbleCommand::issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height, int id)
{
    char name[32];
    sprintf(name,"%04d",id);
    clearappfilerecords();
    applystopallrecord(port);
    applyreferencenoderecord(name,lat*PI/180, lon*PI/180,height, id&1023, id&31);
    base_started=1;

    if(type==RTCM2)
    {
        applyrtcmrecord(port,0x03,0x00,0x19,0x38);
    }
    else if(type==CMR)
    {
        applycmrrecord(port,0x03,0x00,0x00);//0-cmr   3-scmrx
    }
    else if(type==SCMRX)
    {
        applycmrrecord(port,0x03,0x00,0x03);//0-cmr   3-scmrx
    }
    else if(type==DGPS)
    {
        applyrtcmrecord(port,0x03,0x00,0x12,0x38);
    }
    else if(type==RTCM32MSN||type==RTCM3210)//RTCM32
    {
        applyrtcmrecord(port,0x03,0x00,0x31,0);
    }
    else//RTCM3
    {
        applyrtcmrecord(port,0x03,0x00,0x21,0);
    }
     if(!sendbincommand(true))
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init diff data failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::startbase(double lat, double lon, float height,int id)
{
    char name[32];
    sprintf(name,"%04d",id);
    clearappfilerecords();
    applyreferencenoderecord(name,lat*PI/180, lon*PI/180,height, id&1023, id&31);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"startbase failed.");
		return false;
	}
	base_started=1;
	return true;
}

bool cTrimbleCommand::stopbase()
{
    base_started=0;
	return true;
}

bool cTrimbleCommand::checkbasestarted()
{
    if(base_started) return true;
    return false;
}

bool cTrimbleCommand::disableprn(int prn)
{
	if(prn==0) return false;
	clearappfilerecords();
	if(prn>0 && prn<=32)//GPS
	{
		disablegpssnr(prn-1);
	}
	else if(prn>64 && prn<=88) //GLONASS
	{
		disableglonasssnr(prn-65);
	}
	else if(prn>200 && prn<=230) //BEIDOU
	{
		disablebeidousnr(prn-201);
	}
	else if(prn>100 && prn<=136) //Galileo
	{
		disablegalileosnr(prn-101);
	}
	if(!sendbincommand(true))//
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"disableprn failed.");
		return false;
	}
	return true;
}


bool cTrimbleCommand::enableallprn()
{
	clearappfilerecords();
	applyglonassrecord(0);
	applygpsrecord(0);
	applybeidourecord(0);
	applygalileorecord(0);
	if(!sendbincommand(true))//
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"enableallprn failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRTCM23(unsigned char port,int id)
{
    clearappfilerecords();
    applyrtcmrecord(port,0x03,0x00,0x19,0x38);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init rtcm2 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init rtcm failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRTCMV3EPH(unsigned char port, string rtcmv3_eph)
{
    return true;
}

bool cTrimbleCommand::issueRTCM32(unsigned char port,int id)
{
    clearappfilerecords();
    applyrtcmrecord(port,0x03,0x00,0x31,0);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init rtcm3 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init rtcm32 failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRTCM3210(unsigned char port,int id)
{
	issueRTCM32(port,id);
	return true;
}

bool cTrimbleCommand::issueRTCM30(unsigned char port,int id)
{
    clearappfilerecords();
    applyrtcmrecord(port,0x03,0x00,0x21,0);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init rtcm3 failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init rtcm3 failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueROX(unsigned char port,int id)
{
    syslog(LOG_LOCAL7|LOG_ERR,"init rox failed.");
    return false;
}

bool cTrimbleCommand::issueCMR(unsigned char port,int id)
{
    clearappfilerecords();
    applycmrrecord(port,0x03,0x00,0x00);//0-cmr   3-scmrx
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init cmr failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueSCMRX(unsigned char port,int id)
{
    clearappfilerecords();
    applycmrrecord(port,0x03,0x00,0x03);//0-cmr   3-scmrx
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init cmrx failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueDGPS(unsigned char port,double lat, double lon, float height,int id)
{
    clearappfilerecords();
    applyrtcmrecord(port,0x03,0x00,0x12,0x38);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init dgps failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRAWEPH(unsigned char port,int interval, int raw_eph)
{
    int freq;
    freq=getMsgfreq(interval);
    clearappfilerecords();
    applyrt17record(port,freq);//port 1hz
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init raw failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRAW(unsigned char port,int interval, int raw_eph)
{
    int freq;
    freq=getMsgfreq(interval);
    clearappfilerecords();
    applystopallrecord(port);
    applyrt17record(port,freq);//port 1hz
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init raw failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueBINEX(unsigned char port,int interval, int raw_eph)
{
    int freq;
    freq=getMsgfreq(interval);
    clearappfilerecords();
    applystopallrecord(port);
    //applybinexrecord(port,freq);//port 1hz
    applyrt17record(port,freq);//用主机程序实时转
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"init binex failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setElemask(unsigned char elem)
{
    clearappfilerecords();
    applygeneralcontrolsrecord(elem,99);
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set elemask failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::issueRecord(int interval)
{
  unsigned freq=0;
  if(interval == 100)
    freq = 0x01;
  else if(interval == 200)
    freq = 0x02;
  else if(interval == 1000)
    freq = 0x03;
  else if(interval == 2000)
    freq = 0x04;
  else if(interval == 5000)
    freq = 0x05;
  else if(interval == 10000)
    freq = 0x06;
  else if(interval == 30000)
    freq = 0x07;
  else if(interval == 60000)
    freq = 0x08;
  else if(interval == 300000)
    freq = 0x09;
  else if(interval == 600000)
    freq = 0x0a;
  else if(interval == 500)
    freq = 0x0b;
  else if(interval == 15000)
    freq = 0x0c;
  else if(interval == 50)
    freq = 0x0d;
  else if(interval == 20)
    freq = 0x0f;

	clearappfilerecords();
	//applygeneralcontrolsrecord(OEMParam.cutAngle,99);//设置截止角及PDOP
	applyrt17record(OEM_PORT_STATIC,freq);//port 2
	if(!sendbincommand(true))//初始化主板失败
	{
		//printf("start record failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"start record failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::stopRecord()
{
	clearappfilerecords();
	applyrt17record(OEM_PORT_STATIC,0);//port 2
	if(!sendbincommand(true))//初始化主板失败
	{
		syslog(LOG_LOCAL7|LOG_ERR,"stop record failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::stopDiff(unsigned char port)
{
	clearappfilerecords();
	applystopallrecord(port);
	if(!sendbincommand(true))//初始化主板失败
	{
		//printf("stop base failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"stop base failed.");
		return false;
	}
	return true;
}

#include "oem_conf.c"
bool cTrimbleCommand::setDefault()
{
        if(init()==false) return false;

        int n,length = sizeof(data); //配置文件的长度
        unsigned char * ptrstart=data;
        while(length > 0)
        {
            n=FIFO_Add_Buffer(&(m_IPCGPSRAWWrite.IPCFIFO),ptrstart,min(length,1024));
            usleep(1000*200);
            ptrstart +=  n;
            length  -=  n;
        }
        //usleep(1000*500);
        return true;
}


int cTrimbleCommand::getMsgid(OEM_NMEA_MSG_TYPE msgtype)
{
  int msgid;
  switch(msgtype)
  {
  case GGA:
    msgid = 0x06;
    break;
  case GSA:
    msgid = 0x26;
    break;
  case GSV:
    msgid = 0x12;
    break;
  case ZDA:
    msgid = 0x08;
    break;
  case GST:
    msgid = 0x0d;
    break;
 case GLL:
    msgid = 0x2c;
    break;
  case RMC:
    msgid = 40;
    break;
  case VTG:
    msgid = 0x0c;
    break;
 case PJK:
    msgid = 14;
    break;

  default:
    msgid = 0;
  }
  return msgid;
}

int cTrimbleCommand::getMsgfreq(int interval)
{
  int freq;
  if(interval == 100)
    freq = 0x01;
  else if(interval == 200)
    freq = 0x02;
  else if(interval == 1000)
    freq = 0x03;
  else if(interval == 2000)
    freq = 0x04;
  else if(interval == 5000)
    freq = 0x05;
  else if(interval == 10000)
    freq = 0x06;
  else if(interval == 30000)
    freq = 0x07;
  else if(interval == 60000)
    freq = 0x08;
  else if(interval == 300000)
    freq = 0x09;
  else if(interval == 600000)
    freq = 0x0a;
  else if(interval == 500)
    freq = 0x0b;
  else if(interval == 15000)
    freq = 0x0c;
  else if(interval == 50)
    freq = 0x0d;
  else if(interval == 20)
    freq = 0x0f;
  else
    freq = 0;
  return freq;
}

bool cTrimbleCommand::issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval)
{
    int msgid;
    int freq;
    msgid = getMsgid(msgtype);
    freq = 0xff;
    if(trigger==TRIGGER_ONTIME)
    {
      freq = getMsgfreq((int)interval);
    }
    clearappfilerecords();
    applynmearecord(port,freq,0x00,msgid);
    if(!sendbincommand(true))
	{
		//printf("stop base failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"log nmea %d failed.",(int)msgtype);
		return false;
	}
    return true;
}


bool cTrimbleCommand::lognmea(unsigned char port, char *setting)
{
        static const char NMEA_MSG[][8] = {"GGA","GSA","GSV","ZDA","GST","GLL","RMC","VTG","PJK"};
        int msgid;
        int freq=0;
        char *p1,*p2;
        char str[32];
        int  i,n,flag,end,interval;
        int msgnum=9;
        p1=(char *)setting;
        clearappfilerecords();
        applystopallrecord(port);
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
                        msgid = getMsgid((OEM_NMEA_MSG_TYPE)n);
                        freq = getMsgfreq(interval);
                        applynmearecord(port,freq,0x00,msgid);
                    }
                }
                p1=p2+1;
            }
        }

        if(!sendbincommand(true))
        {
            //printf("stop base failed.\r\n");
            syslog(LOG_LOCAL7|LOG_ERR,"log default nmea failed.");
            return false;
        }

        return true;
}

bool cTrimbleCommand::setOemVersion(bool ver7)
{
	return true;
}

bool cTrimbleCommand::init()
{
    commandgetserial();
    if(!sendbincommand(false))//初始化主板失败
	{
	    sleep(1);
	    commandgetserial();
        if(!sendbincommand(false))//初始化主板失败
        {
            //printf("gps init failed.\r\n");
            return false;
        }
	}

    unlogallports();
	return true;
}

bool cTrimbleCommand::setGlonass(int disable)
{
    clearappfilerecords();
    if(disable)
      applyglonassrecord(1);//
    else
      applyglonassrecord(0);//
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set glonass failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setBeidou(int disable)
{
    clearappfilerecords();
    if(disable)
      applybeidourecord(1);//
    else
      applybeidourecord(0);//
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set glonass failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setGPS(int disable)
{
    clearappfilerecords();
    if(disable)
      applygpsrecord(1);//
    else
      applygpsrecord(0);//
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set gps failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setGalileo(int disable)
{
    clearappfilerecords();
    if(disable)
      applygalileorecord(1);//
    else
      applygalileorecord(0);//
    if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set galileo failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setSbas(int disable)
{
    clearappfilerecords();
    if(!disable)
      applygsbasrecord(1);//
    else
      applygsbasrecord(0);//

    if(!sendbincommand(true))//
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set sbas failed.");
		return false;
	}
	return true;
}

bool cTrimbleCommand::setQzss(int disable)
{
	clearappfilerecords();
	if(disable)
		applyqzssrecord(1);//
	else
		applyqzssrecord(0);//
	if(!sendbincommand(true))//初始化主板失败
	{
		//printf("init cmr failed.\r\n");
		syslog(LOG_LOCAL7|LOG_ERR,"set galileo failed.");
		return false;
	}
	return true;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    int written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}
#define  XML_TMP_DIR  "/tmp/"
#define  XML_TMP_FILE  "/tmp970.xml"
bool cTrimbleCommand::GetHttp(const char *url, const char *filename)
{
    FILE *fp; //FILE
    CURL *curl;
    CURLcode res;
    char path[1024];
    sprintf(path,"%s%s",XML_TMP_DIR,filename);
    curl_global_init(CURL_GLOBAL_ALL);
    curl=curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_TIMEOUT ,1);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if((fp=fopen(path,"w"))==NULL)
    {
        curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    if(res!=CURLE_OK)
    {
        if(res==CURLE_OPERATION_TIMEDOUT)
        {
            printf("setting  timeout\n");
        }
        else
        {
            //printf("curl error %d\n",res);
            //syslog(LOG_LOCAL7|LOG_ERR,"curl error %d",res);
        }
        return false;
    }

    return true;
}

bool cTrimbleCommand::txtcommand(char *cmd)
{
    char path[1024];
    sprintf(path,"%s%s",OEM_HTTP,cmd);
    if(!GetHttp((char *)path,XML_TMP_FILE))
    {
        error_count++;
        return false;
    }
	error_count=0;
    return true;
}
