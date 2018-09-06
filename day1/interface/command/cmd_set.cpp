/*
 * cmd_set.cpp
 *
 *  Created on: 2009-7-28
 *      Author: Administrator
 */
#include "command.hpp"
#include "keywords.hpp"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/if.h>
#include "iwlib.h"
#include "IPCInfoUtil.h"
//#include "apue.h"
#include <fcntl.h>

//#include "wireless.h"

using namespace std;

bool command::stopSubProcess()
{
	int i;
	for(i=1;i<MAX_PROCESS;i++)
	{
		if(process[i]>0)
		{
			stop_process(process[i]);
		}
	}

	return true;
}

//it will free argv[x]
//if process_id!=0, start_process will stop the process process_id
bool command::start_process(int argc, char** argv, pid_t &process_id)
{
    int i;
    if(process_id>0)
    {
        stop_process(process_id);
    }
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        perror("fork() error");
        syslog(LOG_LOCAL7|LOG_ERR,"cmd fork() error!");
        return false;
    }
    if (pid == 0)
    {
        argv[argc]=0;
        int ret = execv(argv[ 0] , (char **)argv);
        if (ret < 0)
        {
            perror("execv() error");
            syslog(LOG_LOCAL7|LOG_ERR,"cmd execv() %s error!",argv[ 0]);
        }
        exit(0);
    }
    if (pid > 0)
    {
        process_id=pid ;
        printf("create process: %s %d \n",argv[0], process_id) ;
    }
    for(i=0; i<argc; i++)free(argv[i]);
    return true;
}

bool command::stop_process(pid_t &process_id)
{
    int n=0,status;
    if(process_id)
    {
		syslog(LOG_LOCAL7|LOG_INFO,"stop process %d \n",process_id);
        int ret=kill( process_id,SIGTERM );
        if ( ret )
        {
            //perror( "kill  error" );
            //syslog(LOG_LOCAL7|LOG_ERR,"kill process %d error",process_id);
            process_id=0;
            return false;
        }
        else
        {
            //printf( "kill record process: %d\n", process_id );
            while(waitpid( process_id, &status, WNOHANG )==0&&n++<200)
            {
                usleep(10000);
                //syslog(LOG_LOCAL7|LOG_INFO,"######## %d ########\n",n);
            }

            if(n>=100)
            {
                kill( process_id,SIGKILL );
                syslog(LOG_LOCAL7|LOG_INFO,"######## SIGKILL ########\n");
            }

            process_id=0;
        }
    }
    return true;
}
int command::GetPortXMLValue(const char * szXMLPattern,const char * szPortXMLName,string *value)
{
	char szTemp[50];
	sprintf(szTemp,szXMLPattern,szPortXMLName);
	int iRet=xmlconfig->getValue(szTemp,value);
	transform(value->begin(), value->end(), value->begin(), ::toupper);
	return iRet;
}
//start CMD_BIN_PATH
void command::StartProcess_cmd(const char * szIPCReadInfo,const char * szIPCWriteInfo,int iMailboxId,pid_t &process_id)
{
	char* exec_argv[8] = {0};
	int n;
	exec_argv[0] = BuildInfoFromString(CMD_BIN_PATH);
	exec_argv[1] = BuildInfoFromString(szIPCReadInfo);
	exec_argv[2] = BuildInfoFromString(szIPCWriteInfo);

	n=3;
	exec_argv[3] = (char *)malloc(n+1);
	sprintf(exec_argv[3], "%d",iMailboxId); //
	exec_argv[3][n] = '\0';

	start_process(4,exec_argv,process_id);

}

void command::StartProcess_Met(const char * COMName,const char * baudrate,const char * szIPCWriteInfo,pid_t &process_id)
{
	char* exec_argv[8] = {0};

	exec_argv[0] = BuildInfoFromString(MET_BIN_PATH);
	exec_argv[1] = BuildInfoFromString(COMName);
	exec_argv[2] = BuildInfoFromString(baudrate);
	exec_argv[3] = BuildInfoFromString(szIPCWriteInfo);

	start_process(4,exec_argv,process_id);
}

//szIPCReadInfo: where serial2 read cmd from
//szIPCWriteInfo:where serial2 write response to
void command::StartProcess_Serial2(const char * COMName,const char * baudrate,const char * szIPCReadInfo,const char * szIPCWriteInfo,pid_t &process_id)
{

	char* exec_argv[8] = {0};

	exec_argv[0] = BuildInfoFromString(SERIAL2_BIN_PATH);
	exec_argv[1] = BuildInfoFromString(COMName);
	exec_argv[2] = BuildInfoFromString(baudrate);
	exec_argv[3] = BuildInfoFromString(szIPCReadInfo);
	exec_argv[4] = BuildInfoFromString(szIPCWriteInfo);

	start_process(5,exec_argv,process_id);
}
////read DIFF data from gnss board to szIPCWriteInfo
int command::StartRTKOUTPort( const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
							 const char *szIPCReadInfo ,const char *szIPCWriteInfo)
{
	//read DIFF data from gnss board to szIPCWriteInfo
	if(creat_endpoint(endpoint,iEPProcIdIndex0,0,szIPCReadInfo,szIPCWriteInfo))
	{
		string datatype;
		GetPortXMLValue("PORTS.%s.RTK",szPortXMLName,&datatype);
		if(datatype.length()==0)  datatype="OFF";
		transform(datatype.begin(), datatype.end(), datatype.begin(), ::toupper);
		DIFF_TYPE type;
		if(datatype.compare("RTCM2") ==0) type=RTCM2;
		else if(datatype.compare("RTCM32") ==0) type=RTCM32MSN;
		else if(datatype.compare("RTCM3") ==0) type=RTCM3;
		else if(datatype.compare("CMR") ==0) type=CMR;
		else if(datatype.compare("SCMRX") ==0) type=SCMRX;
		else if(datatype.compare("DGPS") ==0) type=DGPS;
		else if(datatype.compare("ROX") ==0) type=ROX;
		else if(datatype.compare("RTCM32_10") ==0) type=RTCM3210;
		else type=UNDEFINE_DIFFTYPE;
		if(type != UNDEFINE_DIFFTYPE )
		{
			double lat;
			double lon;
			float height;
			int id;
			string value;
			if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELAT",&value)!= RET_OK)
				value="0";
			lat=strtod(value.c_str(),NULL);
			if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELON",&value)!= RET_OK)
				value="0";
			lon=strtod(value.c_str(),NULL);
			if (xmlconfig->getValue("SYSTEM.POSITIONING.BASEHEIGHT",&value)!= RET_OK)
				value="0";
			height=atof(value.c_str());
			if (xmlconfig->getValue("SYSTEM.POSITIONING.BASESITE_ID",&value)!= RET_OK)
                value="0";
            id=atoi(value.c_str());
			if(oem->issueDiffData(endpoint->id,type,lat,lon,height,id))
            {
                if(type==RTCM32MSN)
                {
                    string rtcmv3_eph;
                    GetPortXMLValue("PORTS.%s.RTCMV3_EPH",szPortXMLName,&rtcmv3_eph);
                    if(rtcmv3_eph.length()==0) rtcmv3_eph="";

                    oem->issueRTCMV3EPH(endpoint->id,rtcmv3_eph);
                }

                return 1;
            }
		}
	}
	return 0;
}

//endpoint transfer data from gnss to szIPCWriteBinexInfo,
//then toBinex redecorate data and transfer them to szIPCWriteInfo
int command::StartBinexPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,
							int iEPProcIdIndex0,const char *szIPCReadInfo,const char * szIPCWriteInfo,
							int iBinexProcIdIndex,const char *szIPCWriteBinexInfo,
							string option,int iCheckEPH)
{
	char* exec_argv[8] = {0};
	int n;
	int setup_success=0;
	//read gnss data to szIPCWriteBinexInfo
	if(gpsboard==GPS_UNICORECOMM) szIPCWriteBinexInfo=szIPCWriteInfo;
	if(creat_endpoint(endpoint,iEPProcIdIndex0,0,szIPCReadInfo,szIPCWriteBinexInfo))
	{
		string interval;
		int iInterval;
		GetPortXMLValue("PORTS.%s.RAW",szPortXMLName,&interval);
		if(interval.length()==0)  interval="OFF";
		iInterval=atoi(interval.c_str());

		if (iCheckEPH!=0)
		{
			string raw_eph="0";
			int iRawEPH;
			GetPortXMLValue("PORTS.%s.RAW_EPH",szPortXMLName,&raw_eph);
			iRawEPH = atoi(raw_eph.c_str());
			if( (iInterval!=0)||(iRawEPH !=0))
			{
				if(oem->issueBINEX(endpoint->id,iInterval,iRawEPH))
					setup_success=1;
			}
		}else{
			if( (iInterval!=0))
			{
				if(oem->issueBINEX(endpoint->id,iInterval,-1))
					setup_success=1;
			}
		}
	}
	if(gpsboard==GPS_UNICORECOMM) return setup_success;
	//todo:correct toBinex dataflow
	exec_argv[0] = BuildInfoFromString(BINEX_BIN_PATH);
	exec_argv[1] = BuildInfoFromString(szIPCWriteBinexInfo);//raw gnss data
	exec_argv[2] = BuildInfoFromString(szIPCWriteInfo);//binex data
	exec_argv[3] = BuildInfoFromString(option.c_str());
	start_process(4,exec_argv,process[iBinexProcIdIndex]);
	return setup_success;
}
//szIPCReadInfo:where cmd/diff data read from
//szIPCWriteInfo:where cmd write response/gnss data or gnss data
//return 0:func is found, but process failed; 1:func is found, process sucessful; -1: func is not found
int command::StartPortWithConfigedFunction(const char * szPortXMLName,string func,ENDPOINT_OPT * endpoint,
										   int iEPProcIdIndex0,const char *szIPCReadInfo,const char * szIPCWriteInfo,
										   int iBinexProcIdIndex,const char * szIPCWriteBinexInfo,
										   int iMailboxId, string option,int iCheckEPH)
{
	int setup_success=0;

	if(func.compare("DEBUG") == 0 )
	{
		stop_process(process[PROCESS_COM3]);
		system("echo debug > /boot/debug");
		setup_success=1;
	}
	else if(func.compare("CMD") == 0 )
	{
		setup_success=1;
		StartProcess_cmd(szIPCReadInfo,szIPCWriteInfo,iMailboxId,process[iEPProcIdIndex0]);
	}
	else if(func.compare("NMEA") == 0 )
	{
		//read gnss to szIPCWriteInfo
		if(creat_endpoint(endpoint,iEPProcIdIndex0,0,szIPCReadInfo,szIPCWriteInfo))
		{
			string nmea;
			GetPortXMLValue("PORTS.%s.NMEA",szPortXMLName,&nmea);
			if(nmea.length())
			{
				if(oem->lognmea(endpoint->id,(char *)nmea.c_str()))
					setup_success=1;
			}
		}
	}
	else if(func.compare("RTK_IN") == 0 )
	{
		//read diff data from szIPCReadInfo to gnss board
		if(creat_endpoint(endpoint,iEPProcIdIndex0,0,szIPCReadInfo,szIPCWriteInfo))
		{
			oem->unlogall(endpoint->id);
			oem->rtkmode(endpoint->id);
			setup_success=1;
		}
	}
	else if(func.compare("RTK_OUT") == 0 )
	{
		//read DIFF data from gnss board to szIPCWriteInfo
		setup_success=StartRTKOUTPort(szPortXMLName,endpoint,iEPProcIdIndex0,szIPCReadInfo,szIPCWriteInfo);
	}
	else if(func.compare("RAW") == 0 )
	{
		//read raw gnss data to szIPCWriteInfo
		if(creat_endpoint(endpoint,iEPProcIdIndex0,0,szIPCReadInfo,szIPCWriteInfo))
		{
			string interval;
			GetPortXMLValue("PORTS.%s.RAW",szPortXMLName,&interval);
			if(interval.length()==0)  interval="OFF";
			string raw_eph="0";
			int iRawEPH=-1;
			if (iCheckEPH!=0)
			{
				GetPortXMLValue("PORTS.%s.RAW_EPH",szPortXMLName,&raw_eph);
				iRawEPH = atoi(raw_eph.c_str());
			}
			if(interval.compare("0") !=0||(iRawEPH !=0))
			{
				if(oem->issueRAW(endpoint->id,atoi(interval.c_str()),iRawEPH))
					setup_success=1;
			}
		}
	}
	else if(func.compare("BINEX") == 0 )
	{
		//endpoint transfer data from gnss to szIPCWriteBinexInfo,
		//then toBinex redecorate data and transfer them to szIPCWriteInfo
		setup_success=StartBinexPort(szPortXMLName,endpoint,
			iEPProcIdIndex0,szIPCReadInfo,szIPCWriteInfo,
			iBinexProcIdIndex,szIPCWriteBinexInfo,
			option,iCheckEPH);
	}
	else
	{
		//func is not found
		setup_success=-1;
	}
	return setup_success;
}

//szIPCReadInfo: where program server read data from, the source buffer to send to socket
//szIPCWriteInfo: where program server write to,the dest buffer to write the data from socket
//void command::SetupSocketPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
//							  int iTCPProcIdIndex,const char *szIPCReadInfo,const char * szIPCWriteInfo,
//							  int iBinexProcIdIndex,const char * szIPCWriteBinexInfo, int iMailboxId,
//							  string option)
void command::SetupSocketPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
					 int iTCPProcIdIndex,int iIPCWrite,const char * szIPCReadInfo,
					 int iBinexProcIdIndex,int iIPCBinex, int iMailboxId,string option)
{
	char* exec_argv[8] = {0};
	int tcp_server=0;
	int setup_success=0;
	string  socket_en;
	int funcCMD=0;
	if(process[iEPProcIdIndex0]>0)
	{
		if(release_endpoint(endpoint,iEPProcIdIndex0))
			oem->unlogall(endpoint->id);
	}
	if(process[iTCPProcIdIndex]) stop_process(process[iTCPProcIdIndex]);
	if(process[iBinexProcIdIndex]>0)stop_process(process[iBinexProcIdIndex]);
	GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&socket_en);
	if(socket_en.compare("YES") != 0 )
	{
		if(process[iTCPProcIdIndex]) stop_process(process[iTCPProcIdIndex]);
		setup_success=1;
	}
	else
	{
		string func="";
		GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);

		//根据功能调整fifo的大小
		int iIPCReadSize8K=TCP_DEFAULT_SIZE8K;
		int iIPCBinexSize8K=TCP_DEFAULT_SIZE8K;
		char szIPCWriteInfo[20];
		char szIPCWriteBinexInfo[20];
		if(func.compare("CMD") == 0 )
		{
			iIPCReadSize8K= TCP_CMD_SIZE8K;
			funcCMD=1;
		}
		else if(func.compare("NMEA") == 0 )
		{
			iIPCReadSize8K= TCP_GNSSNMEA_SIZE8K;
		}
		else if(func.compare("RTK_OUT") == 0 )
		{
			iIPCReadSize8K=TCP_GNSSRTKOUT_SIZE8K;
		}
		else if(func.compare("RAW") == 0 )
		{
			iIPCReadSize8K=TCP_GNSSRAW_SIZE8K;
		}
		else if(func.compare("BINEX") == 0 )
		{
			iIPCReadSize8K=TCP_GNSSBINEXOUT_SIZE8K;
			iIPCBinexSize8K=TCP_GNSSBINEXOUT_SIZE8K;
		}
		sprintf(szIPCWriteInfo,"%d-%d",iIPCWrite,iIPCReadSize8K);
		sprintf(szIPCWriteBinexInfo,"%d-%d",iIPCBinex,iIPCBinexSize8K);
		//it will create a endpoint to read gnss data to szIPCWriteInfo
		//szIPCReadInfo:where cmd/diff data read from
		//szIPCWriteInfo:where cmd write response/gnss data or gnss data
		setup_success=StartPortWithConfigedFunction(szPortXMLName,func,endpoint,
			iEPProcIdIndex0,szIPCReadInfo,szIPCWriteInfo,
			iBinexProcIdIndex,szIPCWriteBinexInfo,
			iMailboxId,option,1);

		//read data from szIPCWriteInfo filled by StartPortWithConfigedFunction,send to socket(PC),
		//read data from socket, write to szIPCReadInfo
		string type,tcp_mode,addr;
		GetPortXMLValue("PORTS.%s.MODE",szPortXMLName,&tcp_mode);
		if(tcp_mode.length()==0) tcp_mode="SERVER";

		GetPortXMLValue("PORTS.%s.TYPE",szPortXMLName,&type);
		if(type.length()==0) type="TCP";

		GetPortXMLValue("PORTS.%s.ADDR",szPortXMLName,&addr);
		if(addr.length()==0) addr="6060";

		if(type=="UDP")
		{
			if(tcp_mode=="SERVER")
			{
				tcp_mode="R"+addr;
			}
			else tcp_mode="U"+addr;
		}
		else
		{
			if(tcp_mode=="SERVER")
			{
				tcp_mode="S"+addr;
				tcp_server=1;
			}
			else tcp_mode="C"+addr;
		}

		if(tcp_server)
		{
			exec_argv[0] = BuildInfoFromString(TCP_SERVER_BIN_PATH);
		}
		else
		{
#if(defined GW)
			if(funcCMD)exec_argv[0] = BuildInfoFromString(SOCKET_REMOTE_BIN_PATH);
			else exec_argv[0] = BuildInfoFromString(SOCKET_RAW_BIN_PATH);
#else
			exec_argv[0] = BuildInfoFromString(SOCKET_RAW_BIN_PATH);
#endif
		}

		exec_argv[1] = BuildInfoFromString(tcp_mode.c_str());
		exec_argv[2] = BuildInfoFromString(szIPCWriteInfo); //like IPC_TCP2_READ_INFO
		exec_argv[3] = BuildInfoFromString(szIPCReadInfo); //like IPC_TCP2_WRITE_INFO
		exec_argv[4] = BuildInfoFromString("0");
		start_process(5,exec_argv,process[iTCPProcIdIndex]);

	}
	char szTemp[40];
	sprintf(szTemp,"PORTS.%s.STATUS",szPortXMLName);
	if(setup_success==0)
		xmlconfig->setValue(szTemp,"error");
	else
		xmlconfig->setValue(szTemp,"idle");
	clear_msg(PROC_INTERFACE);
}
int command::Dowork()
{
    if(nodes > 0)
    {
        enPathField enpath =(enPathField)node[0];
        //printf("node0 %d \n", enpath) ;
        switch(enpath)
        {
        case CMD_DEVICE:
            return Device();
        case CMD_SYSTEM:
            return System();
        case CMD_GPS:
            return Gps();
        case CMD_NTRIP:
            return Ntrip();
        case CMD_RECORD:
            return Record();
        case CMD_PORTS:
            return Ports();
        case CMD_NETWORK:
            return Network();
        case CMD_DISK:
            return Disk();
        case CMD_RADIO:
            return Radio();
        case CMD_VPN:
            return Vpn();
        default:
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

int command::Disk()
{
    enPathField enpath =(enPathField)node[1];
    //printf("disk %d,%d \n", CMD_MOUNTMSD,enpath) ;
    switch(enpath)
    {
    case CMD_MOUNTMSD:
        return disk_MountMSD();

    default:
        return -1;
    }
    return -1;
}

int command::disk_MountMSD()
{
    //printf("disk_MountMSD \n") ;
    if(param.compare("YES") == 0)
    {
        param="";
        dev_rec_Stoprec();
        xmlconfig->setValue("DATAFLOW.RECORD.STATUS","break");
        xmlconfig->saveRealtime();
        usb_connected=1;
    }
    else
    {
        if(process[PROCESS_REC]==0)
        {
            xmlconfig->setValue("DATAFLOW.RECORD.STATUS","idle");
            xmlconfig->saveRealtime();
            usb_connected=0;
        }

    }
    return RET_OK;
}

int command::Device()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_SELF_CHECK:
        return dev_SelftCheck();
    case CMD_STARTBASE:
        return dev_StartBase();
    case CMD_STOPBASE:
        return dev_StopBase();
	case CMD_RESTARTBASE:
		return dev_ReStartBase();
    case CMD_RESET:
        return dev_Reset();
    case CMD_FRESET:
        return dev_Freset();
    case CMD_FORMAT:
        return dev_Format();
    case CMD_POWEROFF:
        return dev_PowerOff();
    case CMD_POWER_LEVEL:
        return dev_PowerLevel();
    case CMD_CLEANLOG:
        return dev_CleanLog();
    case CMD_SECURITY_CODE:
        return dev_SecurityCode();
    case CMD_DEVINFO:
        return dev_Devinfo();
    case CMD_REGI:
        return dev_Regi();
    case CMD_UPDATE:
        return dev_Update();
    case CMD_UPGRADE:
        return dev_Upgrade();
    case CMD_UCSHELL:
        return dev_Ucshell();
    case CMD_REMOTE:
        return dev_Remote();
    case CMD_CONFIGSET:
        return dev_Configset();
    default:
        return -1;
    }
    return -1;
}

int command::Record()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_STARTRECORD:
        return dev_rec_Startrec();
    case CMD_STOPRECORD:
        return dev_rec_Stoprec();
	case CMD_PUSH:
		return dev_rec_push();
    case CMD_DELETE:
        return dev_rec_Delete();
    case CMD_ONCHANGED:
        return dev_rec_Onchanged();
	case CMD_GCOMPRESS:
		return dev_rec_gCompress();
    default:
        return -1;
    }
    return -1;
}

int command::dev_Configset()
{
    enPathField enpath =(enPathField)node[2];
    switch(enpath)
    {
    case CMD_SAVE:
        return dev_Configset_Save();
    case CMD_RESTORE:
        return dev_Configset_Restore();
    default:
        return -1;
    }
    return -1;
}

int command::dev_Configset_Save()
{
    syslog(LOG_LOCAL7|LOG_INFO,"save configset\n");
    if(param.length() == 0)
        return RET_ERR_PARAM;

    typedef struct
    {
        char ConfType[64];
        char reverse[12];
    }Tail;
    Tail tail;

    char str[1024]={0};
    string cmd;
    string xml;
    if(param.find("SYSTEM")!=string::npos)
    {
        xml = "network.xml system.xml";
    }else if(param.find("SERVICE")!=string::npos)
    {
        xml = "ports.xml radio.xml ntrip.xml record.xml device.xml vpn.xml";
    }else if(param.find("USER")!=string::npos)
    {
        xml = "users.xml";
    }else if(param.find("ALL")!=string::npos)
    {
        xml = "network.xml system.xml ports.xml radio.xml ntrip.xml record.xml device.xml vpn.xml users.xml";
    }else
    {
        syslog(LOG_LOCAL7|LOG_INFO,"%s: error config request [%s].\n",__FUNCTION__,param.c_str());
        return RET_ERR_PARAM;
    }
    strcpy(tail.ConfType,param.c_str());

    string target = "/tmp/"+param;
    string target_tmp = "/tmp/"+param+"_tmp";

    remove(target.c_str());
    remove(target_tmp.c_str());

    cmd="/usr/bin/tar -zcvf "+target_tmp+" -C /geo/www/config/ "+xml+" 2>&1";
    syslog(LOG_LOCAL7|LOG_INFO," >> %s .\n",cmd.c_str());

    FILE *fp = popen(cmd.c_str(),"r");
    if(NULL != fp)
    {
        while(NULL != fgets(str+strlen(str), sizeof(str)-strlen(str), fp));
        pclose(fp);
        syslog(LOG_LOCAL7|LOG_INFO," >> %s .\n",str);
        /*if(strstr(str,"Error")!=NULL || strstr(str,"ERROR")!=NULL || strstr(str,"error")!=NULL)
        {
            syslog(LOG_LOCAL7|LOG_INFO,"%s: save config file error .\n",__FUNCTION__);
            return RET_EXEC_FAILED;
        }*/

        int file = open(target_tmp.c_str(),O_WRONLY|O_APPEND|O_LARGEFILE,S_IWGRP);
        if (file < 0)
        {
            syslog(LOG_LOCAL7|LOG_ERR,"Unable to open file: %s!",target_tmp.c_str());
            return RET_INVALID_CMD;;
        }
        write(file, &tail,sizeof(tail));
        fsync(file);
        close(file);

        SM4_Encrypt(target_tmp.c_str(),target.c_str());
        remove(target_tmp.c_str());

        return RET_OK;
    }

    return RET_INVALID_CMD;
}

int command::dev_Configset_Restore()
{
    syslog(LOG_LOCAL7|LOG_INFO,"restore configset\n");
    if(param.length() == 0)
        return RET_ERR_PARAM;

    xmlconfig->save_enable = false;

    typedef struct
    {
        char ConfType[64];
        char reverse[12];
    }Tail;
    Tail tail;

    char str[1024]={0};
    string cmd;

    string target = param;
    string target_tmp = param+"_tmp";

    remove(target_tmp.c_str());

    if(SM4_Decrypt(target.c_str(),target_tmp.c_str())<0)
    {
        return RET_EXEC_FAILED;
    }
    remove(target.c_str());

    //检查上传的配置文件类型
    int file = open(target_tmp.c_str(),O_RDONLY,0666);
    if (file < 0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"Unable to open file: %s!",target_tmp.c_str());
        xmlconfig->save_enable = true;
        return RET_INVALID_CMD;
    }
    lseek(file,-sizeof(tail),SEEK_END);
    read(file,&tail,sizeof(tail));
    close(file);
    syslog(LOG_LOCAL7|LOG_INFO," >> need:%s VS load:%s .\n",target.c_str(),tail.ConfType);
    if(strstr(target.c_str(),tail.ConfType)==NULL)
    {
        syslog(LOG_LOCAL7|LOG_INFO," >> Invalid request type : %s .\n",tail.ConfType);
        remove(target_tmp.c_str());
        xmlconfig->save_enable = true;
        return RET_INVALID_VALUE;
    }

    cmd="/usr/bin/tar -xzvf "+target_tmp+" -C /geo/www/config/ 2>&1";
    syslog(LOG_LOCAL7|LOG_INFO," >> %s .\n",cmd.c_str());

    FILE *fp = popen(cmd.c_str(),"r");
    if(NULL != fp)
    {
        while(NULL != fgets(str+strlen(str), sizeof(str)-strlen(str), fp));
        pclose(fp);
        remove(target_tmp.c_str());
        syslog(LOG_LOCAL7|LOG_INFO," >> %s .\n",str);
        if(strstr(str,"Error")!=NULL || strstr(str,"ERROR")!=NULL || strstr(str,"error")!=NULL)
        {
            syslog(LOG_LOCAL7|LOG_INFO,"%s: Restore config file error .\n",__FUNCTION__);
            xmlconfig->save_enable = true;
            return RET_EXEC_FAILED;
        }

        dev_Reset();

        return RET_OK;
    }

    xmlconfig->save_enable = true;
    return RET_INVALID_CMD;
}

int command::dev_Remote()
{
    enPathField enpath =(enPathField)node[2];
    switch(enpath)
    {
    case CMD_RESET:
        return dev_Remote_Reset();
    default:
        return -1;
    }
    return -1;
}

void * UcshellThread(void *p)
{
    string ret;
    string msg;

    pthread_detach(pthread_self());
    if(p==NULL)
    {
        pthread_exit(0);
        return 0;
    }

    command * cmd=(command *)p;
    ret.clear();
    msg.clear();
	ucshell(cmd->ucshell_param,&ret);
    msg+=ret;

    char tmp[32];
    string line;
    if(cmd->ucshell_cmd.length()>0)
    {
        line="@GNSS,"+cmd->ucshell_cmd+",OK";
        //cout<<line<<endl;
        sprintf(tmp,"*%02X\r\n",cmd_crc8((unsigned char   *)line.c_str(),line.length()));
        line.append(tmp);
        msg += line ;
    }
    //syslog(LOG_LOCAL7|LOG_INFO,"ucshell:%ld->%s",cmd->ucshell_source,msg.c_str());
    cmd->CMDSendMessage(cmd->ucshell_source,msg);

    pthread_exit(0);
    return 0;
}


int command::dev_Ucshell()
{
    if(param.length()>0)
    {
        if(param[0] == '\"')
        {
            param=param.substr(1,param.length()-1);
        }
        if(param[param.length()-1] == '\"')
        {
            param=param.substr(0,param.length()-1);
        }
    }
    if(DisableSecurityMode==0)
    {
        if(!(param.compare(0,2,"ps")==0 || param.compare(0,2,"ls")==0 || param.compare(0,3,"cat")==0
                || param.compare(0,8,"ifconfig")==0 || param.compare(0,4,"/geo")==0|| param.compare(0,5,"route")==0       ))
            return RET_INVALID_OPERATION;
    }


    ucshell_cmd=curCommand;
    ucshell_param=param;
    ucshell_source=cmd_source;
    //UcshellThread(NULL);

    if(pthread_create(&ucshell_thread, NULL, UcshellThread, this)!=0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"ucshell() pthread_create error");
    }

    return RET_OK;
}

int command::dev_Remote_Reset()
{
    char* exec_argv[8] = {0};
    unsigned int n;
    string enable,tcp_mode,addr;

    xmlconfig->getValue("DEVICE.REMOTE.ENABLE",&enable);
    if(enable.length()==0)enable="YES";
    transform(enable.begin(), enable.end(), enable.begin(), ::toupper);

    stop_process(process[PROCESS_REMOTE]);
    stop_process(process[PROCESS_REMOTE_C]);

    if(enable.compare("YES")!=0&&enable.compare("ENABLE")!=0)
    {
        return RET_OK;
    }

	StartProcess_cmd(IPC_REMOTE_WRITE_INFO,IPC_REMOTE_READ_INFO,CMD_REMOTE_ID,process[PROCESS_REMOTE_C]);

	//sleep(1);

    xmlconfig->getValue("DEVICE.REMOTE.ADDR",&addr);
    if(addr.length()==0) addr="122.13.16.137:6070";
    tcp_mode="C"+addr;

    exec_argv[0] = BuildInfoFromString(SOCKET_REMOTE_BIN_PATH);
    exec_argv[1] = BuildInfoFromString(tcp_mode.c_str());
	exec_argv[2] = BuildInfoFromString(IPC_REMOTE_READ_INFO);
	exec_argv[3] = BuildInfoFromString(IPC_REMOTE_WRITE_INFO);
	exec_argv[4] = BuildInfoFromString("0");
    start_process(5,exec_argv,process[PROCESS_REMOTE]);

    return RET_OK;
}

int command::dev_Update()
{
    char* exec_argv[8] = {0};
    unsigned int n;
    string model,ver,serial;
    string remoteAddr,deviceInfo;

    syslog(LOG_LOCAL7|LOG_INFO,"dev_Update ---> %s .\n",param.c_str());

    xmlconfig->getValue("DEVICE.REMOTE.ADDR",&remoteAddr);
    if(remoteAddr.length()==0)
    {
        if(param.length()==0)
        {
            remoteAddr="58.62.206.155:6075";
        }
        else
        {
            remoteAddr = replace_all(param,"|",":");
            xmlconfig->setValue("DEVICE.REMOTE.ADDR",remoteAddr);
        }
    }

//    if(param.length()==0)
//    {
//        xmlconfig->getValue("DEVICE.REMOTE.ADDR",&remoteAddr);
//        if(remoteAddr.length()==0) remoteAddr="122.13.16.137:6070";
//    }else{
//        remoteAddr = replace_all(param,"|",":");
//        xmlconfig->setValue("DEVICE.REMOTE.ADDR",remoteAddr);
//    }
    remoteAddr = "C"+remoteAddr;

    deviceInfo.clear();
    xmlconfig->getValue("DEVICE.INFO.MODEL",&model);
    xmlconfig->getValue("DEVICE.INFO.APPVERSION",&ver);
    xmlconfig->getValue("DEVICE.INFO.SERIAL",&serial);
    deviceInfo=model+"|"+ver+"|"+serial;

    exec_argv[0] = BuildInfoFromString(REMOTEUPGRADE_BIN_PATH);
    exec_argv[1] = BuildInfoFromString(remoteAddr.c_str());
	exec_argv[2] = BuildInfoFromString(IPC_REMOTE_READ_INFO);
    exec_argv[3] = BuildInfoFromString(deviceInfo.c_str());

    start_process(4,exec_argv,process[PROCESS_UPGRADE]);

    return RET_OK;
}

int command::dev_BatchUpdate()
{
    string value,addr;
    string account,password;
    string strFirmVer,strSerial;
    char user_pass[256]={0},chSendBuffer[256]={0};
    //int write=0;
    //size_t pos;

    if (xmlconfig->getValue("DEVICE.BATCH_UPDATE.PORT",&value)!= RET_OK||value.length()==0)
    {
        value="9009";
        xmlconfig->setValue("DEVICE.BATCH_UPDATE.PORT",value.c_str());
    }
    addr="S"+value;

//    if (xmlconfig->getValue("DEVICE.UPDATE.ACCOUNT",&account)!= RET_OK||account.length()==0)
//        account="user";
//
//    if (xmlconfig->getValue("DEVICE.UPDATE.PASSWORD",&password)!= RET_OK||password.length()==0)
//        password="password";

    if (xmlconfig->getValue("GPS.INFO.SERIAL",&account)!= RET_OK||account.length()==0)
        account="NSC200XXX";

    if (xmlconfig->getValue("GPS.INFO.FIRMWARE_VER",&password)!= RET_OK||password.length()==0)
        password="2017XXXX";

    sprintf(user_pass,"%s|%s",account.c_str(),password.c_str());

    if (xmlconfig->getValue("DEVICE.INFO.APPVERSION",&strFirmVer)!= RET_OK)
        strFirmVer="";

    if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&strSerial)!= RET_OK)
        strSerial="";

    sprintf(chSendBuffer, "NSC200|%s|%s",strFirmVer.c_str(),strSerial.c_str());

    char* exec_argv[8] = {0};
    unsigned int /*n,*/len;

    exec_argv[0] = BuildInfoFromString(BATCHUPDATE_BIN_PATH);
    exec_argv[1] = BuildInfoFromString(addr.c_str());
    exec_argv[2] = BuildInfoFromString(user_pass);
    exec_argv[3] = BuildInfoFromString(chSendBuffer);
    start_process(4,exec_argv,process[PROCESS_BATCHUPDATE]);

    return RET_OK;
}

bool default_set = 0;

int command::dev_Upgrade()
{

#define UPGRADE_PCT           "/tmp/PCT"
#define UPGRADE_UPT           "/tmp/UPT"

#define CHECK_UPGRADE         "/geo/app/decompress"
#define UPGRADE_FILE          "/geo/sd/update/update.bin"

#define UHF_UPGRADE_BIN_PATH  "/geo/app/satel_update"
#define UHF_UPGRADE_PARM_PATH "/tmp/update/SATELLINE"

#define XDL_UPGRADE_BIN_PATH  "/geo/app/xdl_update"
#define XDL_UPGRADE_PARM_PATH "/tmp/update/XDLFW"

#define TRM_UPGRADE_BIN_PATH  "/geo/app/trm_update"
#define TRM_UPGRADE_PARM_PATH "/tmp/update/TRMFW"


#define GSM_UPGRADE_BIN_PATH  "/geo/app/phs_update"
#define GSM_UPGRADE_PARM_PATH "/tmp/update/PHS"
#define GSM_SERIAL            "ttyACM0"

//GPS_TRIMBLE
#define OEM_UPGRADE_BIN_PATH  "/geo/app/bd970_update"
#define OEM_FILE_BD970 "/tmp/update/BDFW"
//GPS_HEMISPHERE
#define OEM_HEMISPHERE_UPGRADE_BIN_PATH  "/geo/app/hems_update"
#define OEM_UPGRADE_PARM_PATH "/tmp/update/update.bin"
//GPS_NOVATEL
#define OEM_NOVATEL_UPGRADE_BIN_PATH  "/geo/app/novatel_update"
#define OEM_FILE_NOVATEL "/tmp/update/NOVFW"

#define OEM_SERIAL            "ttyO2"

#define SCRIPT_UPGRADE_BIN_PATH  "/geo/app/script_update"

    transform(param.begin(), param.end(), param.begin(), ::toupper);
    syslog(LOG_LOCAL7|LOG_INFO,"-------> %s .\n",param.c_str());
    //int show_msg=0;
    unsigned int i;
    char* exec_argv[8] = {0};
    unsigned int n;
    int len,error;
    char tmp[512]={0};
    string exec;
    string path,upgrade_bin_path;
    string model;
    if(param=="CHECK")
    {
        path.assign(UPGRADE_FILE);
        if((access(path.c_str(),F_OK|R_OK))!=0)
        {
            sprintf(tmp,"ERROR: not found update file!");
            file_write(UPGRADE_UPT,(unsigned char *)tmp,strlen(tmp));
            return RET_EXEC_FAILED;
        }
        syslog(LOG_LOCAL7|LOG_INFO,"update file check.\n");
        if((access(UPGRADE_UPT,F_OK|R_OK))==0)
        {
            unlink(UPGRADE_UPT);
        }
        if((access(UPGRADE_PCT,F_OK|R_OK))==0)
        {
            unlink(UPGRADE_PCT);
        }

        exec_argv[0] = BuildInfoFromString(CHECK_UPGRADE);
        exec_argv[1] = BuildInfoFromString(UPGRADE_FILE);
        start_process(2,exec_argv,process[PROCESS_UPGRADE]);

        error=1;

        for(i=0; i<30; i++)
        {
            sleep(1);
            clear_msg(PROC_INTERFACE);
            if((access(UPGRADE_UPT,F_OK|R_OK))==0)
            {
                len=file_read(UPGRADE_UPT,(unsigned char *)tmp,sizeof(tmp));
                if(len>0)
                {
                    error=0;
                    break;
                }
            }
        }
        if(!error)
        {
            memset(tmp,0,sizeof(tmp));
            len=file_read(UPGRADE_UPT,(unsigned char *)tmp,sizeof(tmp));
            if(len>0)
            {
                if(strstr(tmp,"ERROR")>0)
                {
                    unlink(UPGRADE_FILE);
                    error=1;
                    syslog(LOG_LOCAL7|LOG_INFO,"update file check fail.\n");
                }
                else
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"update file check success.\n");
                }
            }
            else
            {
                sprintf(tmp,"ERROR: update file check timeout!");
                file_write(UPGRADE_UPT,(unsigned char *)tmp,strlen(tmp));
                unlink(UPGRADE_FILE);
            }
        }

        if(error) return RET_EXEC_FAILED;

        param.assign(tmp);
        transform(param.begin(), param.end(), param.begin(), ::toupper);

        syslog(LOG_LOCAL7|LOG_INFO,"=====> %s .\n",param.c_str());

        return RET_OK;
    }
    else if(param=="APP" || param=="OEM" || param=="UHF"|| param=="GSM"|| param=="PHS" || param=="SATEL"|| param=="SCRIPT")
    {
		ctl_setDataLink(MODE_LINK_1);
        //default_cpld_mode();
        default_set=0;
        error=0;
        sleep(1);
        if(param=="APP")
        {
            sleep(1);
            dev_Reset();
            return RET_OK;
        }
        else if((param=="UHF"||param=="SATEL") && SysParam.radio)
        {
            xmlconfig->getValue("RADIO.INFO.MODEL",&model);
            if(model.find("SATEL")!=string::npos)
            {
                exec.assign(UHF_UPGRADE_BIN_PATH);
                path.assign(UHF_UPGRADE_PARM_PATH);
            }
            else if(model.find("XDL")!=string::npos)
            {
                exec.assign(XDL_UPGRADE_BIN_PATH);
                path.assign(XDL_UPGRADE_PARM_PATH);
            }
            else if(model.find("TRM")!=string::npos)
            {
                exec.assign(TRM_UPGRADE_BIN_PATH);
                path.assign(TRM_UPGRADE_PARM_PATH);
            }
            else
            {
                sprintf(tmp,"ERROR: Unsupport model!");
                file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                return RET_EXEC_FAILED;
            }

            if((access(path.c_str(),F_OK|R_OK))!=0)
            {
                return RET_EXEC_FAILED;
            }

            syslog(LOG_LOCAL7|LOG_INFO,"UHF upgrade.\n");
            if((access(UPGRADE_PCT,F_OK|R_OK))==0)
            {
                unlink(UPGRADE_PCT);
            }
            system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
            if(model.find("TRM")!=string::npos)
            {
                system((char *)"insmod /lib/modules/uhf-config.ko  1> /dev/null  2>&1");
                system((char *)"rmmod uhf-config  1> /dev/null  2>&1");
            }
            stop_process(process[PROCESS_RADIO]);
            sleep(1);
            init_oled_upgrade();
            //PROCESS_UPGRADE
            exec_argv[0] = BuildInfoFromString(exec.c_str());
			if(SysParam.hardver>=4)
			{
				sprintf(tmp,"/dev/%s",RADIO_SERIAL_V4);
			}
			else
			{
				sprintf(tmp,"/dev/%s",RADIO_SERIAL);
			}
            exec_argv[1] =BuildInfoFromString(tmp);
            exec_argv[2] = BuildInfoFromString(path.c_str());
            start_process(3,exec_argv,process[PROCESS_UPGRADE]);


            error=1;
            for(i=0; i<15; i++)
            {
                sleep(1);
                clear_msg(PROC_INTERFACE);
                if((access(UPGRADE_PCT,F_OK|R_OK))==0)
                {
                    error=0;
                    break;
                }
            }

            if(!error)
            {
                n=600;//600s
                for(i=0; i<n; i++)
                {
                    clear_msg(PROC_INTERFACE);
                    memset(tmp,0,sizeof(tmp));
                    len=file_read(UPGRADE_PCT,(unsigned char *)tmp,sizeof(tmp));
                    if(len>0)
                    {
                        if(strstr(tmp,"ERROR")>0)
                        {
                            error=1;
                            break;
                        }
                        else if(strstr(tmp,"SUCCESS")>0)
                        {
                            error=0;
                            break;
                        }
                    }
                    sleep(1);
                }
                if(i>=n)
                {
                    error=1;
                    sprintf(tmp,"ERROR: Upgrade timeout!");
                    file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                }
            }
            else
            {
                sprintf(tmp,"ERROR: Upgrade program error!");
                file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
            }
#if 1
            //selfcheck
            if(!error)
            {
                string radio_en;
                xmlconfig->getValue("PORTS.RADIO.ENABLE",&radio_en);
                xmlconfig->setValue("PORTS.RADIO.ENABLE","YES");

                system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
                sleep(1);
                system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
                radio_Reset();
                sleep(1);
                if(radio_selfcheck()==RET_OK)
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"Selfcheck: UHF ok.\n");
                }
                else
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"Selfcheck: UHF failed!\n");
                }
                clear_msg(PROC_INTERFACE);
                system((char *)"rmmod  uhf-enable  1> /dev/null  2>&1") ;

                xmlconfig->setValue("PORTS.RADIO.ENABLE",radio_en);
            }
#endif
            init_oled();
            default_setting();

        }
        else if(param=="PHS" || param=="GSM" )
        {
            path.assign(GSM_UPGRADE_PARM_PATH);
            if((access(path.c_str(),F_OK|R_OK))!=0)
            {
                return RET_EXEC_FAILED;
            }

            xmlconfig->getValue("NETWORK.INFO.MODEL",&model);
            if(model.find("PHS")==string::npos)
            {
                sprintf(tmp,"ERROR: Unsupport model!");
                file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                return RET_EXEC_FAILED;
            }

            syslog(LOG_LOCAL7|LOG_INFO,"GSM upgrade.\n");
            if((access(UPGRADE_PCT,F_OK|R_OK))==0)
            {
                unlink(UPGRADE_PCT);
            }

            system("killall pppd  1> /dev/null  2>&1");
            init_oled_upgrade();

            //PROCESS_UPGRADE
            exec_argv[0] = BuildInfoFromString(GSM_UPGRADE_BIN_PATH);
            sprintf(tmp,"/dev/%s",GSM_SERIAL);
            exec_argv[1] = BuildInfoFromString(tmp);
            exec_argv[2] = BuildInfoFromString(path.c_str());
            start_process(3,exec_argv,process[PROCESS_UPGRADE]);

            error=1;
            for(i=0; i<30; i++)
            {
                sleep(1);
                clear_msg(PROC_INTERFACE);
                if((access(UPGRADE_PCT,F_OK|R_OK))==0)
                {
                    error=0;
                    break;
                }
            }
            if(!error)
            {
                n=900;//900s
                for(i=0; i<n; i++)
                {
                    clear_msg(PROC_INTERFACE);
                    memset(tmp,0,sizeof(tmp));
                    len=file_read(UPGRADE_PCT,(unsigned char *)tmp,sizeof(tmp));
                    if(len>0)
                    {
                        if(strstr(tmp,"ERROR")>0)
                        {
                            error=1;
                            break;
                        }
                        else if(strstr(tmp,"SUCCESS")>0)
                        {
                            error=0;
                            break;
                        }
                    }
                    sleep(1);
                }
                if(i>=n)
                {
                    error=1;
                    sprintf(tmp,"ERROR: Upgrade timeout!");
                    file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                }
            }
            else
            {
                sprintf(tmp,"ERROR: Upgrade program error!");
                file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
            }
#if 0
            //selfcheck
            if(!error)
            {
                sleep(3);
                if(net_selfcheck()==RET_OK)
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"Selfcheck: GSM/3G ok.\n");
                }
                else
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"Selfcheck: GSM/3G failed!\n");
                }
                clear_msg(PROC_INTERFACE);
            }
#endif
           init_oled();
           default_setting();

        }
        else if(param=="OEM" || param=="GPS" )
        {
            if(gpsboard==GPS_HEMISPHERE)
            {
                path.assign(OEM_UPGRADE_PARM_PATH);
                upgrade_bin_path.assign(OEM_HEMISPHERE_UPGRADE_BIN_PATH);
            }
            else if(gpsboard==GPS_NOVATEL)
            {
                path.assign(OEM_FILE_NOVATEL);
                upgrade_bin_path.assign(OEM_NOVATEL_UPGRADE_BIN_PATH);
            }
            else if(gpsboard==GPS_TRIMBLE)
            {
                path.assign(OEM_FILE_BD970);
                upgrade_bin_path.assign(OEM_UPGRADE_BIN_PATH);
            }
            else return RET_EXEC_FAILED;

            if((access(path.c_str(),F_OK|R_OK))!=0)
            {
                return RET_EXEC_FAILED;
            }
            syslog(LOG_LOCAL7|LOG_INFO,"OEM upgrade.\n");
            if((access(UPGRADE_PCT,F_OK|R_OK))==0)
            {
                unlink(UPGRADE_PCT);
            }

            //ctl_setDataLink(MODE_LINK_UHF);
            init_oled_upgrade();

            if(gpsboard==GPS_HEMISPHERE)
            {
                //if(SysParam.hardver!=1)
                oem->txtcommand((char *)"$JOFF,PORTB");
                oem->txtcommand((char *)"$JOFF,PORTA");
            }
            stop_process(process[PROCESS_OEM]);
            direct_link=1;
            //PROCESS_UPGRADE
            exec_argv[0] = BuildInfoFromString(upgrade_bin_path.c_str());
            sprintf(tmp,"/dev/%s",OEM_SERIAL);
            exec_argv[1] = BuildInfoFromString(tmp);
            exec_argv[2] = BuildInfoFromString(path.c_str());
            //syslog(LOG_LOCAL7|LOG_INFO,"OEM upgrade...%s %s %s...\n",exec_argv[0],exec_argv[1],exec_argv[2]);
            start_process(3,exec_argv,process[PROCESS_UPGRADE]);

            error=1;
            for(i=0; i<30; i++)
            {
                sleep(1);
                clear_msg(PROC_INTERFACE);
                if((access(UPGRADE_PCT,F_OK|R_OK))==0)
                {
                    error=0;
                    break;
                }
            }
            if(!error)
            {
                n=1800;//1800s
                for(i=0; i<n; i++)
                {
                    clear_msg(PROC_INTERFACE);
                    memset(tmp,0,sizeof(tmp));
                    len=file_read(UPGRADE_PCT,(unsigned char *)tmp,sizeof(tmp));
                    if(len>0)
                    {
                        if(strstr(tmp,"ERROR")>0)
                        {
                            error=1;
                            break;
                        }
                        else if(strstr(tmp,"SUCCESS")>0)
                        {
                            error=0;
                            break;
                        }
                    }
                    sleep(1);
                }
                if(i>=n)
                {
                    error=1;
                    sprintf(tmp,"ERROR: Upgrade timeout!");
                    file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                }
            }
            else
            {
                sprintf(tmp,"ERROR: Upgrade program error!");
                file_write(UPGRADE_PCT,(unsigned char *)tmp,strlen(tmp));
                //reset oem
                system("rmmod oem_enable 1> /dev/null  2>&1");
                sleep(1);
                system("insmod /lib/modules/oem-enable.ko 1> /dev/null  2>&1");
            }
            //selfcheck
            clear_msg(PROC_INTERFACE);
            syslog(LOG_LOCAL7|LOG_INFO,"OEM upgrade completed.\n");
            clear_msg(PROC_INTERFACE);
            sleep(3);
            init_gps();
            clear_msg(PROC_INTERFACE);
            sleep(3);
            clear_msg(PROC_INTERFACE);
            direct_link=0;
            init_oled();
            default_setting();
            ntrip_Reset();
        }

        if(error) return RET_EXEC_FAILED;
        return RET_OK;
    }
    else if(param=="SCRIPT")
    {
        path.assign(UPGRADE_FILE);
        if((access(path.c_str(),F_OK|R_OK))!=0)
        {
            sprintf(tmp,"ERROR: not found update file!");
            file_write(UPGRADE_UPT,(unsigned char *)tmp,strlen(tmp));
            return RET_EXEC_FAILED;
        }
        syslog(LOG_LOCAL7|LOG_INFO,"Script upgrade.\n");
        if((access(UPGRADE_PCT,F_OK|R_OK))==0)
        {
            unlink(UPGRADE_PCT);
        }

        //PROCESS_UPGRADE
        exec_argv[0] = BuildInfoFromString(SCRIPT_UPGRADE_BIN_PATH);
        exec_argv[1] = BuildInfoFromString(path.c_str());
        start_process(2,exec_argv,process[PROCESS_UPGRADE]);

    }
    else if(param == "DELETE")
    {
        if((access(UPGRADE_FILE,F_OK|R_OK))==0)
        {
            unlink(UPGRADE_FILE);
        }
        return RET_OK;
    }

    return RET_ERR_PARAM;

}


int command::dev_Reset()
{
    param="";
    dev_rec_Stoprec();
    system("killall network");

    xmlconfig->saveConfig();
    xmlconfig->save_enable = false;

    sync();
    sleep(2);
    cout<< "system will be reset."<<endl;
    syslog(LOG_LOCAL7|LOG_INFO,"(%s) system will be reset.",APP_VERSION);
    system("reboot");
    return RET_OK;
}

int command::dev_Freset()
{
    xmlconfig->save_enable = false;

    oem->setDefault();
    system("rm -rf /geo/app/config/antennalist.xml");
    system("rm -rf /geo/app/config/dataflow.xml");
    system("rm -rf /geo/app/config/device.xml");
    system("rm -rf /geo/app/config/gps.xml");
    system("rm -rf /geo/app/config/ntrip.xml");
    system("rm -rf /geo/app/config/ports.xml");
    system("rm -rf /geo/app/config/record.xml");
    system("rm -rf /geo/app/config/servers.xml");
    system("rm -rf /geo/app/config/system.xml");
    system("rm -rf /geo/app/config/users.xml");
    system("rm -rf /geo/app/config/radio.xml");
    system("rm -rf /geo/app/config/vpn.xml");

    sync();

	string cmd;
	cmd=std::string("echo \"Listen ")+DEF_HTTP_PORT+"\"> "+HTTP_PORT_PATH;
	system(cmd.c_str());

    dev_Reset();

    return RET_OK;
}

int command::dev_Format()
{
    transform(param.begin(), param.end(), param.begin(), ::toupper);
    string path;
    path=param;
    param="";

    dev_rec_Stoprec();

    if(path=="SDCARD")
    {
        system("umount /geo/sd1 && umount /geo/ftp && umount /media/mmcblk1p1");
        system("mkfs.vfat -F 32 /dev/mmcblk1p1");
        //system("mount -t vfat -o umask=0000 /dev/mmcblk0p4  /media/mmcblk1p1");
        system("mount /dev/mmcblk1p1 /geo/ftp && mount /dev/mmcblk1p1 /geo/sd1");
        system("mkdir -p /geo/sd1/record");
		system("mkdir -p /geo/sd1/update");
        //system("/geo/app/scripts/fatlabel  /dev/mmcblk1p1  ZENITH35_SD");
    }
    else
    {
        system("umount /geo/sd && umount /geo/ftp && umount /media/mmcblk0p4");
        system("umount /dev/mmcblk0p4");
        system("mkfs.vfat -F 32 /dev/mmcblk0p4");
        system("mount -t vfat -o umask=0000 /dev/mmcblk0p4  /media/mmcblk0p4");
        system("mount /dev/mmcblk0p4 /geo/ftp && mount /dev/mmcblk0p4 /geo/sd");
        system("mount /dev/mmcblk0p4 /geo/ftp && mount /dev/mmcblk0p4 /geo/sd");
		system("rm -rf /etc/record_session/*");
        system("mkdir -p /geo/sd/record");
		system("mkdir -p /geo/sd/update");
    }

    //dev_rec_Startrec();

    return RET_OK;
}

int command::dev_PowerOff()
{
    param="";
    dev_rec_Stoprec();
    xmlconfig->saveConfig();
    xmlconfig->save_enable = false;
    sync();
    sleep(1);
    cout<< "system will be shutdown."<<endl;
    syslog(LOG_LOCAL7|LOG_INFO,"system will be shutdown .\n");

    device_poweroff();

    return RET_OK;
}

int command::dev_SecurityCode()
{
    if(param=="geo110310")
    {
        DisableSecurityMode=1;
    }
    else
    {
        DisableSecurityMode=0;
    }
    return RET_OK;
}

int command::dev_Devinfo()
{
    unsigned char data[2048];
    unsigned int len;

    if(DisableSecurityMode==0)  return RET_UNSUPPORTED_NOW;

    if(param.length()>0)
    {
        if(param[0] == '\"')
        {
            param=param.substr(1,param.length()-1);
        }
        if(param[param.length()-1] == '\"')
        {
            param=param.substr(0,param.length()-1);
        }

        memset(data,0,sizeof(data));
        len=Sting2Hex(param.c_str(),param.length(),data,sizeof(data));
		syslog(LOG_LOCAL7|LOG_INFO,">>%s",data);
        if(len>0)
        {
            if(strstr((char *)data,"[DEVICE_SERIAL]")>0)
            {
                 len+=1;
                 if(eeprom_write(DEV_INFO_ADDR,data,len))
                 {
                    return RET_OK;
                 }
                 else
                 {
                    return RET_EXEC_FAILED;
                 }
            }
        }
    }
    return RET_ERR_PARAM;
}


int command::dev_PowerLevel()
{
    xmlconfig->setValue("DEVICE.POWER_LEVEL",param);
    xmlconfig->saveConfig();
    sync();
    return RET_OK;
}


int command::dev_Regi()
{
    //transform(param.begin(), param.end(), param.begin(), ::toupper);
    unsigned int Temp_ExpireDateTime=0;

#ifdef CheckRegi
if(checkreg==1)
{
    if(param.length()<32) return RET_ERR_PARAM;
    string regi=param.substr(0,32);

    string serial;
    if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&serial)!= RET_OK) serial="0000000000000";

    if(!test_authcode((char *)regi.c_str(),(char *)serial.c_str(),&Temp_ExpireDateTime,&SysParam.DeviceOption))
    {
        xmlconfig->setValue("SYSTEM.EXPIREDATE","0");
        xmlconfig->setValue("SYSTEM.OPTION","0");
        xmlconfig->setValue("SYSTEM.STATE","ERROR");
        xmlconfig->saveConfig();
        SysParam.reg_state=REG_ERROR;
        return RET_ERR_PARAM;
    }
    string value;
    char tmp[128];
    char tmp_NowDateTime[10];
    SysParam.ExpireDateTime=Temp_ExpireDateTime;
    sprintf(tmp,"%d",SysParam.ExpireDateTime);
    value.assign(tmp);
    xmlconfig->setValue("SYSTEM.EXPIREDATE",value);

    sprintf(tmp,"%d",SysParam.DeviceOption);
    value.assign(tmp);
    xmlconfig->setValue("SYSTEM.OPTION",value);

    value.assign((char *)regi.c_str());
    xmlconfig->setValue("SYSTEM.AUTHCODE",value);

    xmlconfig->getValue("GPS.TIME.GPSDATE",&value);
    if(value.length()!=10)
    {
        SysParam.reg_state=REG_CHECKING;
        xmlconfig->setValue("SYSTEM.STATE","CHECKING");
        xmlconfig->setValue("DEVICE.STATUS","0111");
    }
    else
    {
        strcpy(tmp_NowDateTime,value.c_str());
        SysParam.NowDateTime=(tmp_NowDateTime[0]-'0')*10000000;
        SysParam.NowDateTime+=(tmp_NowDateTime[1]-'0')*1000000;
        SysParam.NowDateTime+=(tmp_NowDateTime[2]-'0')*100000;
        SysParam.NowDateTime+=(tmp_NowDateTime[3]-'0')*10000;
        SysParam.NowDateTime+=(tmp_NowDateTime[5]-'0')*1000;
        SysParam.NowDateTime+=(tmp_NowDateTime[6]-'0')*100;
        SysParam.NowDateTime+=(tmp_NowDateTime[8]-'0')*10;
        SysParam.NowDateTime+=(tmp_NowDateTime[9]-'0');

            if(SysParam.NowDateTime<20000000||SysParam.NowDateTime==0)
            {
                SysParam.reg_state=REG_CHECKING;
                xmlconfig->setValue("SYSTEM.STATE","CHECKING");
                xmlconfig->setValue("DEVICE.STATUS","0111");
            }
            else if(SysParam.NowDateTime<=SysParam.ExpireDateTime)
            {
                SysParam.reg_state=REG_OK;
                xmlconfig->setValue("SYSTEM.STATE","NORMAL");
                xmlconfig->setValue("DEVICE.STATUS","0");
            }
            else
            {
                SysParam.reg_state=REG_EXPIRED;
                xmlconfig->setValue("SYSTEM.STATE","EXPIRED");
                xmlconfig->setValue("DEVICE.STATUS","0111");
                param="";
                dev_rec_Stoprec();
                dev_StopBase();
                xmlconfig->setValue("PORTS.BLUETOOTH.ENABLE","NO");
                xmlconfig->setValue("PORTS.COM1.ENABLE","NO");
                xmlconfig->setValue("PORTS.NTRIP_CLIENT.ENABLE","NO");
                xmlconfig->setValue("PORTS.SOCKET.ENABLE","NO");
                xmlconfig->setValue("PORTS.SOCKET1.ENABLE","NO");
                xmlconfig->setValue("PORTS.SOCKET2.ENABLE","NO");
                port_Reset();
                set_ntrip_disconnect(0);
                set_ntrip_disconnect(1);
                set_ntrip_disconnect(2);
                set_ntrip_disconnect(3);
            }
        }

    xmlconfig->saveConfig();

    if(!update_authcode((char *)regi.c_str()))
        return RET_EXEC_FAILED;
//    SysParam.reg_state=REG_CHECKING;
}
#endif
//   init_sysmode();
    return RET_OK;
}

int command::dev_SelftCheck()
{
        string cmd;
        string value;
        string lang;
        //if(process[PROCESS_SELF_CHECK]) stop_process(process[PROCESS_SELF_CHECK]);

        if (xmlconfig->getValue("SYSTEM.STATUS",&value)!= RET_OK)
            value="idle";

        if(value=="selftest")
            return RET_INVALID_OPERATION;

        if(xmlconfig->getValue("SYSTEM.LANG", &lang)!= RET_OK)
            lang="en";

        if(lang=="ru")
            system("echo \"Режим самопроверки активирован...\" >/opt/log/selftest.log ");
        else
            system("echo \"selftest starting...\" >/opt/log/selftest.log ");

        xmlconfig->setValue("SYSTEM.STATUS","selftest");
        xmlconfig->saveConfig();
        pid_t pid;
        pid=fork();
        if (pid == -1)
        {
            perror("fork() error");
            syslog(LOG_LOCAL7|LOG_ERR,"dev_SelftCheck fork()  error!");
            return RET_EXEC_FAILED;
        }

        cmd.assign(SELFTEST_SCRIPT);

        if (pid == 0)
        {
            system(cmd.c_str());
            system("/usr/bin/geo/message \"SET,SYSTEM.STATUS,idle\"");
            system("/usr/bin/geo/message \"SAVE\"");
            exit(0);
        }
        if (pid > 0)
        {
            process[PROCESS_SELF_CHECK]=pid ;
        }

        return RET_OK;
}


int command::dev_CleanLog()
{
    transform(param.begin(), param.end(), param.begin(), ::toupper);
    if(param.compare("APP") == 0 ||param.compare("GEO") == 0 )
    {
        system("rm /opt/log/geo.log");
        system("touch /opt/log/geo.log");
    }
    else if(param.compare("SYS") == 0 ||param.compare("OS") == 0 )
    {
        system("rm /opt/log/sys.log");
        system("touch /opt/log/sys.log");
    }
    else if(param.compare("KERN") == 0 )
    {
        system("rm /opt/log/kern.log");
        system("touch /opt/log/kern.log");
    }
    else if(param.compare("VPN") == 0 )
    {
        ;
    }
    else
    {
        system("rm /opt/log/geo.*");
        system("rm /opt/log/sys.*");
        system("rm /opt/log/kern.*");
        system("touch /opt/log/geo.log");
        system("touch /opt/log/sys.log");
        system("touch /opt/log/kern.log");
    }
    return RET_OK;
}

static int highInterval=100000;
static int interval_2hz=0,interval_5hz=0;
int command::dev_rec_Startrec()
{
    if(param.length()>0)
    {
        set_record_start(param);
    }else{
        char tmp[128];
        string value;
        xmlconfig->getList("RECORD.SESSION@NAME",&value);
        std::vector<std::string> x = split(value.c_str(), '|');
        for(int i=0;i<(int)x.size();i++)
        {
            sprintf(tmp,"%s",x.at(i).c_str());
            set_record_start(tmp);
        }
    }

    return RET_OK;
}

int command::set_record_start(string name)
{
    char* exec_argv[8] = {0};
    unsigned int n;
    char tmp[256] = {0};

#ifdef CheckRegi
	if(checkreg==1)
	{
		if(SysParam.reg_state!=REG_OK)
		{
			syslog(LOG_LOCAL7|LOG_INFO,"set_record_start %s | reg_state!=REG_OK \n",name.c_str());
			return RET_INVALID_OPERATION;
		}
	}
#endif
    //syslog(LOG_LOCAL7|LOG_INFO,"set_record_start:%d > %s",name.length(),name.c_str());
    //if(process[PROCESS_REC]>0) stop_process(process[PROCESS_REC]);
    //if(process[PROCESS_REC_TCP]>0) stop_process(process[PROCESS_REC_TCP]);

	string strSessionName;
	string strSessionStatus;

	sprintf(tmp,"%s",name.c_str());
	strSessionName.assign("RECORD.SESSION@NAME:");
	strSessionName.append(tmp);

	strSessionStatus="DATAFLOW."+strSessionName+".STATUS";

	if(usb_connected)
	{
		xmlconfig->setValue(strSessionStatus,STR_STATUS_USB_CONNECTED);
		return RET_UNSUPPORTED_NOW;
	}

    RECORDSESSION_OPT session;
    if(!creat_recsession(&session,(char *)name.c_str()))
        return RET_UNSUPPORTED_NOW;
    syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession: key:%d name:%s",session.key,name.c_str());

    string paht,filename,interval,path;
    string duration;
    string push,autorec,intpointrec,rinex,binex,compress,option,protocol;
	string oem_option;

	char szIPCRawData[20];
	FormatIPCInfo(szIPCRawData,IPC_TCP_RECORD,IPC_RECORD_SIZE8K);

    if (xmlconfig->getValue(strSessionName+".INTERVAL",&interval)!= RET_OK)
        interval="1000";

	int temp_interval=atoi(interval.c_str());

	if(temp_interval==500)interval_2hz=1;
	if(temp_interval==200)interval_5hz=1;
	syslog(LOG_LOCAL7|LOG_INFO,"# interval_2hz:%d interval_5hz:%d temp_interval:%d,highInterval:%d",interval_2hz,interval_5hz,temp_interval,highInterval);
	//以低于1s的频率输出，而又没有更高频率输出时，统一申请1s的输出
	if(temp_interval>=1000&&highInterval>1000)temp_interval=1000;
	//2hz 和 5hz 同时输出，而又没有更高频输出时，申请10hz的输出
	if(interval_2hz&&interval_5hz&&temp_interval>=200&&highInterval>200)temp_interval=100;
	syslog(LOG_LOCAL7|LOG_INFO,"@ interval_2hz:%d interval_5hz:%d temp_interval:%d,highInterval:%d",interval_2hz,interval_5hz,temp_interval,highInterval);

	//xmlconfig->saveRealtime();
	if(use_raw_port)
	{
		if(temp_interval<highInterval)
		{
			syslog(LOG_LOCAL7|LOG_INFO,"LOG raw port interval:%d",temp_interval);
			if(!oem->issueRecord( temp_interval) )
			//char tempStr[128];
			//sprintf(tempStr,"$JASC,GPGGA,%.5f\r\n",((float)1000/(float)atoi(interval.c_str())));
			//if(!oem->txtcommand(tempStr))
			{
				xmlconfig->setValue(strSessionStatus,STR_STATUS_SET_INTERVAL_ERR);
				return RET_EXEC_FAILED;
			}
			highInterval=temp_interval;
		}
	}
	else
	{
		ENDPOINT_OPT endpoint;
		int iCreateEndpoint=0;
		if(process[PROCESS_REC_TCP]==0)
		{
			if(!creat_endpoint(&endpoint,PROCESS_REC_TCP,0,IPC_TCP_RECORD_WRITE_INFO,szIPCRawData))
			{
				printf("no ep: %s\n",strSessionStatus.c_str());
				xmlconfig->setValue(strSessionStatus,STR_STATUS_NO_FREE_EP);
				return RET_UNSUPPORTED_NOW;
			}
			iCreateEndpoint=1;
		}else{
			if(!find_endpoint(&endpoint,PROCESS_REC_TCP))
			{
				xmlconfig->setValue(strSessionStatus,"can not find endpoint");
				syslog(LOG_LOCAL7|LOG_INFO,"Not find PROCESS_REC_TCP ");
			}
		}

		//syslog(LOG_LOCAL7|LOG_INFO,"interval:%d highInterval:%d",temp_interval,highInterval);
		if(temp_interval<highInterval)
		{
			syslog(LOG_LOCAL7|LOG_INFO,"LOG interval:%d",temp_interval);
			if(!oem->issueRAW(endpoint.id, temp_interval,-1))
			{
				xmlconfig->setValue(strSessionStatus,STR_STATUS_SET_INTERVAL_ERR);
				//all record share this endpoint, don't release this endpoint
				if (iCreateEndpoint==1)
				{
					release_endpoint(&endpoint,PROCESS_REC_TCP);
				}
				return RET_EXEC_FAILED;
			}
			highInterval=temp_interval;
		}
	}

    if (xmlconfig->getValue(strSessionName+".PAHT",&paht)!= RET_OK)
        paht="";
    if (xmlconfig->getValue(strSessionName+".FILENAME",&filename)!= RET_OK)
        filename="FILE0001";
    cout<< "record filename: "<<filename<<endl;
    if(filename.compare("ssssdddf.yyt")==0)
    {
        filename="[SSSSDOYX].dat";
        xmlconfig->setValue(strSessionName+".FILENAME",filename);
    }

    xmlconfig->getValue(strSessionName+".DURATION",&duration);
    if (duration.length()==0) duration="0";

    if (xmlconfig->getValue(strSessionName+".PUSH.ENABLE",&push)!= RET_OK)
        if (push.length()==0) push="NO";
    transform(push.begin(), push.end(), push.begin(), ::toupper);
	if (xmlconfig->getValue(strSessionName+".PUSH.PROTOCOL",&protocol)!= RET_OK)
		if (protocol.length()==0) protocol="FTP";
	transform(protocol.begin(), protocol.end(), protocol.begin(), ::toupper);
    if (xmlconfig->getValue(strSessionName+".AUTO_REC",&autorec)!= RET_OK)
        if (autorec.length()==0) autorec="NO";
    transform(autorec.begin(), autorec.end(), autorec.begin(), ::toupper);

    if (xmlconfig->getValue(strSessionName+".RINEX",&rinex)!= RET_OK)
        if (rinex.length()==0) rinex="NO";
    transform(rinex.begin(), rinex.end(), rinex.begin(), ::toupper);
    if (xmlconfig->getValue(strSessionName+".BINEX",&binex)!= RET_OK)
        if (binex.length()==0) binex="NO";
    transform(binex.begin(), binex.end(), binex.begin(), ::toupper);

    if (xmlconfig->getValue(strSessionName+".INTEGRAL_POINT_REC",&intpointrec)!= RET_OK)
        if (intpointrec.length()==0) intpointrec="NO";
    transform(intpointrec.begin(), intpointrec.end(), intpointrec.begin(), ::toupper);
	if (xmlconfig->getValue("RECORD.GCOMPRESS",&compress)!= RET_OK)
		if (compress.length()==0) compress="Off";
	transform(compress.begin(), compress.end(), compress.begin(), ::toupper);

    option="_";
	oem_option="_";
	if(push=="YES"){
		if(protocol == "FTP") option+="FTPPUSH";
		else if(protocol == "GEO"){
		    string geomode;
            if (xmlconfig->getValue(strSessionName+".GEO.MODE",&geomode)!= RET_OK)
            if (geomode.length()==0) geomode="CLIENT";
            transform(protocol.begin(), protocol.end(), protocol.begin(), ::toupper);

            if(geomode == "CLIENT")option+="GEOPUSHC";
            else if(geomode == "SERVER")option+="GEOPUSHS";
		}
	}
    if(autorec=="YES") option+="_AUTO";
//    if(rinex=="YES") option+="_RINEX";
//    else if(rinex=="RINEX302") option+="_RINEX302";
//    else if(rinex=="RINEX210") option+="_RINEX210";
//    if(binex=="YES") option+="_BINEX";
    if(intpointrec=="YES") option+="_INTREC";
	if(compress!="OFF") option+=("_"+compress);

	string gpsmodel;
	xmlconfig->getValue("DEVICE.INFO.GPSBOARD",&gpsmodel);
	option+="_OEM-"+gpsmodel;
	oem_option="_OEM-"+gpsmodel;

    path=RECORD_PATH+paht+filename;
    //const char* exec_argv[ 128] = {"/usr/bin/geo/record", "/tmp/test.dat", "8301", 0};
	char szIPCRecData[20];
	FormatIPCInfo(szIPCRecData,session.key,IPC_RECORD_SIZE8K);

	exec_argv[0] = BuildInfoFromString(RECORD_BIN_PATH);
	exec_argv[1] = BuildInfoFromString(szIPCRecData);

	//MessageQueueId
	n=3;
	exec_argv[2] = (char *)malloc(n+1);
	sprintf(exec_argv[2], "%d",PROC_DECODER);

	exec_argv[3] = BuildInfoFromString(path.c_str());
	exec_argv[4] = BuildInfoFromString(duration.c_str());
	exec_argv[5] = BuildInfoFromString(name.c_str());
	exec_argv[6] = BuildInfoFromString(option.c_str());
	exec_argv[7] = BuildInfoFromString(IPC_MET_WRITE_INFO);
	start_process(8,exec_argv,process[PROCESS_REC+session.id]);
	xmlconfig->setValue("DATAFLOW.RECORD.PATH",RECORD_PATH);



	if(process[PROCESS_TRANSPOND]==0)
	{
		exec_argv[0] = BuildInfoFromString(RAW_TRANSPOND_BIN_PATH);

		if(use_raw_port)
		{
			exec_argv[1] = BuildInfoFromString(RECORD_IPC_INFO);
		}
		else
		{
			exec_argv[1] = BuildInfoFromString(szIPCRawData);
		}

		exec_argv[2] = BuildIPCInfo(IPC_TRANSPOND_CMD,IPC_CMD_SIZE8K);
		exec_argv[3] = BuildIPCInfo(IPC_TRANSPOND_CMD_RESPONSE,IPC_CMD_SIZE8K);

		n=20;
		exec_argv[4] = (char *)malloc(n+1);
		sprintf(exec_argv[4],"%s:%s",szIPCRecData,interval.c_str());

		exec_argv[5] = BuildInfoFromString(oem_option.c_str());

		start_process(6,exec_argv,process[PROCESS_TRANSPOND]);
		usleep(200*1000);
	}

	string cmd;
	memset(tmp,0,sizeof(tmp));
	sprintf(tmp,"ADD,%s:%s\r\n",szIPCRecData,interval.c_str());
	cmd.assign(tmp);

	string ret;
	ipc_command(IPC_TRANSPOND_CMD,IPC_TRANSPOND_CMD_RESPONSE,
		(unsigned char *)cmd.c_str(),cmd.length(),&ret,5);

	if(ret.find("OK")!=std::string::npos)
	{
		syslog(LOG_LOCAL7|LOG_INFO,"%s > %s OK",__FUNCTION__,cmd.c_str());
	}


	record_enable=1;
	return RET_OK;
}

int command::dev_rec_Stoprec()
{
    if(param.length()>0)
    {
        set_record_stop(param);
    }else{
        char tmp[128];
        string value;
        xmlconfig->getList("RECORD.SESSION@NAME",&value);
        std::vector<std::string> x = split(value.c_str(), '|');
        for(int i=0;i<(int)x.size();i++)
        {
            sprintf(tmp,"%s",x.at(i).c_str());
            set_record_stop(tmp);
        }
//        char *p = strtok((char *)value.c_str(), "|");
//        while(p)
//        {
//            sprintf(tmp,"%s",p);
//            str.assign("RECORD.SESSION@NAME:");
//            str.append(tmp);
//            syslog(LOG_LOCAL7|LOG_INFO,"%s",str.c_str());
//
//            set_record_stop(tmp);
//
//            p = strtok(NULL, "|");
//        }
    }

    return RET_OK;
}

int command::set_record_stop(string name)
{
    char tmp[128];
    string strSessionName;
	string str;
//syslog(LOG_LOCAL7|LOG_INFO,"set_record_stop:%d > %s",name.length(),name.c_str());
    RECORDSESSION_OPT session;
    if(release_recsession(&session,(char *)name.c_str()))
        if(!stop_process(process[PROCESS_REC+session.id])) ;//return RET_EXEC_FAILED;

    if(recSessionEmpty)
    {
		stop_process(process[PROCESS_TRANSPOND]);
        if(use_raw_port)
        {
            if(!oem->stopRecord());
        }
        else
        {
            ENDPOINT_OPT endpoint;
            release_endpoint(&endpoint,PROCESS_REC_TCP);
            oem->unlogall(endpoint.id);
        }
		highInterval=100000;
		interval_2hz=0,interval_5hz=0;
    }
    syslog(LOG_LOCAL7|LOG_INFO,"===Stop session name:%s recSessionEmpty:%d ========",name.c_str(),recSessionEmpty);
    sprintf(tmp,"%s",name.c_str());
    strSessionName.assign("RECORD.SESSION@NAME:");
    strSessionName.append(tmp);
    //syslog(LOG_LOCAL7|LOG_INFO,"#### %s",str.c_str());

    xmlconfig->setValue("DATAFLOW."+strSessionName+".STATUS","stop");
    xmlconfig->setValue("DATAFLOW."+strSessionName+".SIZE","0");

    xmlconfig->getValue("DATAFLOW.RECORD.SESSION@NAME:*.STATUS",&str);
    if(!strstr(str.c_str(),"recording"))
    {
        xmlconfig->setValue("DATAFLOW.RECORD.STATUS","stop");
        record_enable=0;
    }

    xmlconfig->saveRealtime();
    return RET_OK;
}
int command::dev_rec_Delete()
{
    string file;
    if(param.length())
    {
        file=RECORD_PATH+param;
        if(strstr(param.c_str(),"..")>0) return RET_ERR_PARAM;

        if(isdir(file.c_str()))
        {
            if(deletedir(file.c_str())<0) return RET_EXEC_FAILED;
            //syslog(LOG_LOCAL7|LOG_WARNING,"User delete dir: %s\n",file.c_str());
        }
        else
        {
            if(unlink(file.c_str())<0)  return RET_EXEC_FAILED;
            //syslog(LOG_LOCAL7|LOG_WARNING,"User delete file: %s\n",file.c_str());
        }


        sync();
        return RET_OK;
    }
    return RET_ERR_PARAM;
}

int command::dev_rec_push()
{
	enPathField enpath =(enPathField)node[2];
	switch(enpath)
	{
	case CMD_FTP:
		return dev_rec_ftp_Push();
	case CMD_GEO:
		return dev_rec_geo_Push();
	default:
		return -1;
	}
	return -1;
}

int command::dev_rec_ftp_Push()
{
    string cmd;
    string file;
    string task_file;
	char pushpath[128];
	size_t  pos;
	string session,sessionName;

	syslog(LOG_LOCAL7|LOG_INFO,"ftp_Push: %s\n",param.c_str());
    if(param.length()>0)
    {
        //if(process[PROCESS_FTP]) stop_process(process[PROCESS_FTP]);
		session.assign("RECORD");
        file=param;
		if((pos=file.find("|"))!=string::npos)
		{
			session.clear();

			sessionName = file.substr(0,pos);
			file = file.substr(pos+1,file.length());

			session.assign("RECORD.SESSION@NAME:");
			session += sessionName;
		}

        //printf("param %s\n",param.c_str());
        if(param!="*")
        {
			if(strstr(param.c_str(),"..")>0){
				syslog(LOG_LOCAL7|LOG_INFO,"%s: invalid file/dir \n",file.c_str());
				return RET_ERR_PARAM;
			}
            if((access(file.c_str(),F_OK|R_OK))<0)
            {
                struct stat info;
                stat(file.c_str(),&info);
                if(!S_ISDIR(info.st_mode))
				{
					syslog(LOG_LOCAL7|LOG_INFO,"%s not exist \n",file.c_str());
                    return RET_ERR_PARAM;
				}
            }
        }

		string addr,port,user,pass,path;
		xmlconfig->getValue(session+".FTP.ADDR",&addr);
		if(addr.length()==0) return RET_EXEC_FAILED;
		xmlconfig->getValue(session+".FTP.PORT",&port);
		if(port.length()==0) return RET_EXEC_FAILED;
		xmlconfig->getValue(session+".FTP.USER",&user);
		if(user.length()==0) return RET_EXEC_FAILED;
		xmlconfig->getValue(session+".FTP.PASS",&pass);
		if(pass.length()==0) return RET_EXEC_FAILED;
		xmlconfig->getValue(session+".FTP.PATH",&path);
		if(path.length()==0) path="./";

		cmd.assign(FTP_PUSH_SCRIPT);
		cmd=cmd+" "+addr+" "+port+" "+user+" "+pass+" "+path+" "+file+" "+"\n";

		system("mkdir -p /etc/record_session/ftp_task/");
		task_file.assign("/etc/record_session/ftp_task/");
		task_file=task_file+sessionName+".txt";

        pid_t pid;
        pid=fork();
        if (pid == -1)
        {
            perror("fork() error");
            syslog(LOG_LOCAL7|LOG_ERR,"cmd dev_rec_ftp_Push fork()  error!");
            return RET_EXEC_FAILED;
        }

        if (pid == 0)
        {
			syslog(LOG_LOCAL7|LOG_INFO,"%s\n",cmd.c_str());
            //system(cmd.c_str());
            int ftp_fp;
            int val;
            ftp_fp = open(task_file.c_str(),O_RDWR|O_CREAT|O_APPEND ,0666);
            val=fcntl(ftp_fp,F_GETFL,0);
            fcntl(ftp_fp,F_SETFL,val|O_DSYNC);
            if(!write(ftp_fp,cmd.c_str(),cmd.length() ) )
                syslog(LOG_LOCAL7|LOG_ERR,"cmd dev_rec_ftp_Push write()  error!");
            //syslog(LOG_LOCAL7|LOG_INFO,"cmd dev_rec_ftp_Push write(%s) !",cmd.c_str());
            close(ftp_fp);
            exit(0);
        }
        if (pid > 0)
        {
            process[PROCESS_FTP]=pid ;
        }

    }
    else
        return RET_ERR_PARAM;
    return RET_OK;
}

int command::dev_rec_geo_Push()
{
	string cmd;
	string file;
	size_t  pos;
	string session,sessionName;
	static unsigned char id=0;
	char buffer[512]={0};
	char szSessionFileLogName[1024]={0};

	//syslog(LOG_LOCAL7|LOG_INFO,"geo_Push: %s\n",param.c_str());
	if(param.length()>0)
	{
		session.clear();
		sessionName=param;
		if((pos=param.find("|"))!=string::npos)
		{
			sessionName = param.substr(0,pos);
			file = param.substr(pos+1,param.length());
		}
        session.assign("RECORD.SESSION@NAME:");
        session += sessionName;

		//检测是否有正在发相同时段文件的GEO_PUSH进程
		/*sprintf(buffer, "ps -w | grep %s | grep %s | grep -v grep | wc -l",GEO_PUSH,sessionName.c_str());
		FILE *fp = popen(buffer,"r");
		if(NULL != fp)
		{
			memset(buffer,0,sizeof(buffer));
			if(fgets(buffer,sizeof(buffer),fp) != NULL){
				buffer[strlen(buffer) - 1] = '\0';
				if(strlen(buffer)>0 && atoi(buffer)>=1)
				{
					//syslog(LOG_LOCAL7|LOG_INFO,"already pushing :%s\n",sessionName.c_str());
					pclose(fp);
					return RET_OK;
				}
			}
		}
		pclose(fp);*/

		//printf("param %s\n",param.c_str());
		if(param!="*")
		{
//			if(strstr(param.c_str(),"..")!=NULL){
//				syslog(LOG_LOCAL7|LOG_INFO,"%s: invalid file/dir \n",file.c_str());
//				return RET_ERR_PARAM;
//			}
//			if(file.length()>0 && access(file.c_str(),F_OK|R_OK))<0)
//			{
//				syslog(LOG_LOCAL7|LOG_INFO,"%s not exist \n",file.c_str());
//				return RET_ERR_PARAM;
//			}

			string addr,port,enable,protocol;
			xmlconfig->getValue(session+".PUSH.ENABLE",&enable);
			if(enable.length()==0){
				enable="NO";
			}
			xmlconfig->getValue(session+".PUSH.PROTOCOL",&protocol);
			if(protocol.length()==0){
				protocol="FTP";
			}
			xmlconfig->getValue(session+".GEO.ADDR",&addr);
			if(addr.length()==0){
				syslog(LOG_LOCAL7|LOG_INFO,"%s: invalid addr \n",addr.c_str());
				return RET_EXEC_FAILED;
			}
			xmlconfig->getValue(session+".GEO.PORT",&port);
			if(port.length()==0){
				syslog(LOG_LOCAL7|LOG_INFO,"%s: invalid port \n",port.c_str());
				return RET_EXEC_FAILED;
			}

			string mode = "C"+addr+":"+port;

			sprintf(buffer, "ps -w | grep %s | grep %s | grep -v grep | awk '{print $6}'",GEO_PUSH,sessionName.c_str());
			FILE *fp = popen(buffer,"r");
			if(NULL != fp)
			{
				memset(buffer,0,sizeof(buffer));
				if(fgets(buffer,sizeof(buffer),fp) != NULL){
					buffer[strlen(buffer) - 1] = '\0';
					if(strlen(buffer)>0 && strcmp(buffer,mode.c_str())==0)
					{
						//syslog(LOG_LOCAL7|LOG_INFO,"already pushing :%s\n",sessionName.c_str());
						pclose(fp);
						return RET_OK;
					}
				}
			}
			pclose(fp);
			memset(buffer,0,sizeof(buffer));
			sprintf(buffer, "kill `ps -w | grep %s | grep %s | grep -v grep | awk '{print $1}'`",GEO_PUSH,sessionName.c_str());
			system(buffer);

			pid_t pid;
			pid=fork();
			if (pid == -1)
			{
				perror("fork() error");
				syslog(LOG_LOCAL7|LOG_ERR,"fork() error !");
				return false;
			}
			if (pid == 0)
			{
				char* exec_argv[4] = {0};
				exec_argv[0] = BuildInfoFromString(GEO_PUSH);
				exec_argv[1] = BuildInfoFromString(mode.c_str());
				exec_argv[2] = BuildInfoFromString(sessionName.c_str());
				exec_argv[3] = 0;

				int ret = execv(exec_argv[ 0] , (char **)exec_argv);
				if (ret < 0)
				{
					perror("execv() error");
					syslog(LOG_LOCAL7|LOG_ERR,"execv() %s error !",exec_argv[ 0]);
				}

				exit(0);
			}
			if (pid > 0)
			{
				printf("create : %s \n", GEO_PUSH) ;
			}
		}
	}
	else
		return RET_ERR_PARAM;
	return RET_OK;
}

int command::dev_rec_Onchanged()
{
    char tempStr[128];
    char portStr[32];
	string value;
    ENDPOINT_OPT endpoint;

	if(use_raw_port)
	{
		endpoint.id=0;
	}
    else
	{
		if(!find_endpoint(&endpoint,PROCESS_REC_TCP)){
			syslog(LOG_LOCAL7|LOG_INFO,"%s: Not find PROCESS_REC_TCP ",__FUNCTION__);
			return RET_EXEC_FAILED;
		}
		//syslog(LOG_LOCAL7|LOG_INFO,"%s >> find id:%d",__FUNCTION__,endpoint.id);
	}

    oem->issueRAWEPH(endpoint.id,highInterval,-1);

    return RET_OK;
}

int command::dev_rec_gCompress()
{
	char tmp[128];
	string strSessionName,value;

	if(param.length()==0)
		return RET_ERR_PARAM;

	xmlconfig->setValue("RECORD.GCOMPRESS",param);

	xmlconfig->getList("RECORD.SESSION@NAME",&value);
	std::vector<std::string> x = split(value.c_str(), '|');
	for(int i=0;i<(int)x.size();i++)
	{
		sprintf(tmp,"%s",x.at(i).c_str());

		strSessionName.assign("RECORD.SESSION@NAME:");
		strSessionName.append(tmp);

		xmlconfig->getValue("DATAFLOW."+strSessionName+".STATUS",&value);
		if(value.compare("recording")==0)
		{
			set_record_stop(tmp);
			//syslog(LOG_LOCAL7|LOG_INFO,"%s\n",strSessionName.c_str());
			xmlconfig->setValue("DATAFLOW."+strSessionName+".STATUS","idle");
			xmlconfig->setValue("DATAFLOW."+strSessionName+".STARTTIME","");
			xmlconfig->setValue("DATAFLOW."+strSessionName+".SIZE","0");
			xmlconfig->setValue("DATAFLOW."+strSessionName+".PATH","");
		}
	}
	xmlconfig->saveRealtime();

	return RET_OK;
}

int command::dev_ReStartBase()
{
	if(oem->checkbasestarted())
	{
		dev_StartBase();
	}

	return RET_OK;
}

int command::dev_StartBase()
{
    string datatype;
#ifdef CheckRegi
if(checkreg==1)
{
    if(SysParam.reg_state!=REG_OK)
        return RET_INVALID_OPERATION;
}
#endif
    //if(base_started) return RET_OK;

    int id=0;
    double lat;
    double lon;
    float height;
    string value;
    if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELAT",&value)!= RET_OK)
        return RET_EXEC_FAILED;
    lat=strtod(value.c_str(),NULL);
    if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELON",&value)!= RET_OK)
        return RET_EXEC_FAILED;
    lon=strtod(value.c_str(),NULL);
    if (xmlconfig->getValue("SYSTEM.POSITIONING.BASEHEIGHT",&value)!= RET_OK)
        return RET_EXEC_FAILED;
    height=atof(value.c_str());
    if (xmlconfig->getValue("SYSTEM.POSITIONING.BASESITE_ID",&value)!= RET_OK)
        value="0";
    id=atoi(value.c_str());
#ifdef ANTENNA_MANAGEMENT
    int antenna_H,antenna_HL1;
    xmlconfig->getValue("ANTENNA.H",&value);
    antenna_H=atoi(value.c_str());
    xmlconfig->getValue("ANTENNA.HL1",&value);
    antenna_HL1=atoi(value.c_str());
    height=(float)(antenna_H+antenna_HL1)/10000 + height;
#endif

    if(!oem->startbase(lat,lon,height,id)){
        if(!oem->startbase(lat,lon,height,id))
            return RET_EXEC_FAILED;
    }

    syslog(LOG_LOCAL7|LOG_INFO,"startbase lat: %.9f, lon: %.9f, height: %.3f\n",lat,lon,height);

#ifdef ANTENNA_MANAGEMENT
    setBaseAntennaModel();
#endif
    //diff serial
    //usr/bin/geo/serial ttyS0 8101 8102
    base_started=1;
    return RET_OK;
}


int command::dev_StopBase()
{
    oem->stopbase();
    base_started=0;
    return RET_OK;
}


int command::Gps()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_SELF_CHECK:
        if(oem->setDefault())
            return RET_OK;
        else
            return RET_EXEC_FAILED;
	case CMD_DISABLEPRN:
		return gps_DisablePrn();
	case CMD_ENABLEALLPRN:
		return gps_EnableAllPrn();
    default:
        return -1;
    }
    return -1;
}

int command::set_gps_disableprn(string opt)
{
#define MAX_SATS  512
	int i,prn;
	char *strValue[MAX_SATS];

	if(!oem->enableallprn()) return RET_EXEC_FAILED;
	if(opt.length()==0)
	{
		return RET_OK;
	}
	string val=opt;
	int n=_split((char *)val.c_str(),'|',strValue,MAX_SATS);
	//printf("n=%d\n",n);
	if(n>0)
	{
		for(i=0;i<n;i++)
		{
			//printf("%d: %s\n",i,strValue[i]);
			prn=0;
			if(*strValue[i]=='G'||*strValue[i]=='R'||*strValue[i]=='C'||*strValue[i]=='E')
			{
				if(*strValue[i]=='G') prn=atoi(strValue[i]+1);
				else if(*strValue[i]=='R') prn=atoi(strValue[i]+1)+64;
				else if(*strValue[i]=='C') prn=atoi(strValue[i]+1)+200;
				else if(*strValue[i]=='E') prn=atoi(strValue[i]+1)+100;
			}
			else
			{
				prn=atoi(strValue[i]);
			}
			//printf("dis prn: %d\n",prn);
			if(prn>0)
			{
				if(!oem->disableprn(prn)) return RET_EXEC_FAILED;
			}
		}
	}

	return RET_OK;
}

int command::gps_DisablePrn()
{
	int res=RET_OK;
	transform(param.begin(), param.end(), param.begin(), ::toupper);
	res=set_gps_disableprn(param);
	xmlconfig->setValue("GPS.DISABLEPRN",param);
	return res;
}

int command::gps_EnableAllPrn()
{
	int res=RET_OK;
	if(!oem->enableallprn())
		res=RET_EXEC_FAILED;
	xmlconfig->setValue("GPS.DISABLEPRN","");
	return res;
}


int command::Ntrip()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_CONNECT:
        return ntrip_connect();
    case CMD_DISCONNECT:
        return ntrip_disconnect();
    default:
        return -1;
    }
    return -1;
}

int command::ntrip_connect()
{
//    int res;
    //cout<<param<<endl;

    if(param.length()>0)
    {
        set_ntrip_connect(atoi(param.c_str()));
    }
    else
    {
        set_ntrip_connect(0);
        set_ntrip_connect(1);
        set_ntrip_connect(2);
    }

    return RET_OK;

}

void command::CheckAndStartNtripDoubleInCOM1(const char * szIPCNtripSource)
{
	char* exec_argv[8] = {0};
	string enable,func;
	xmlconfig->getValue("PORTS.COM1.ENABLE",&enable);
	xmlconfig->getValue("PORTS.COM1.FUNCTION",&func);
	if(enable.compare("YES") == 0 && func.find("NTRIP")!=std::string::npos)
	{
		exec_argv[0] = BuildInfoFromString(HUB_BIN_PATH);
		exec_argv[1] = BuildInfoFromString(szIPCNtripSource);
		exec_argv[2] = BuildInfoFromString(IPC_COM_WRITE_INFO);
		start_process(3,exec_argv,process[PROCESS_HUB]);
	}else{
		stop_process(process[PROCESS_HUB]);
	}
}
int command::ntrip_disconnect()
{
    if(param.length()>0)
    {
        set_ntrip_disconnect(atoi(param.c_str()));
    }
    else
    {
        set_ntrip_disconnect(0);
        set_ntrip_disconnect(1);
        set_ntrip_disconnect(2);
    }
    return RET_OK;
}

int command::set_ntrip_connect(int id)
{
    int res;
   // NTRIP_SOCKET_PORT
    unsigned char socket_id=NTRIP_SOCKET_ID-id;
    char tmp[256];
    string str;
	string strDataFlowId;
    char* exec_argv[8] = {0};
    unsigned int n;

#ifdef CheckRegi
	if(checkreg==1)
	{
		if(SysParam.reg_state!=REG_OK)
		{
			syslog(LOG_LOCAL7|LOG_INFO,"set_ntrip_connect %d | reg_state!=REG_OK \n",id);
			return RET_INVALID_OPERATION;
		}
	}
#endif

    res=dev_StartBase();
    if(res!=RET_OK) return res;

    ENDPOINT_OPT endpoint;
    string datatype;
	char ipcReadInfo[20];

	sprintf(tmp,"%d",id);
	strDataFlowId.assign("NTRIP.CONNECTION@ID:");
	strDataFlowId.append(tmp);

	if (gpsboard==GPS_HEMISPHERE)
	{
		//always use ep iNtripEP0ProcessId, it is the data source of all ntrip process
		int iNtripEP0ProcessId=PROCESS_NTRIP+1;
		int iIPCNtripSource=IPC_NTRIP_SOCKET_READ+5;
		//char ipcReadInfo[20];
		char ipcWriteInfo[20];
		sprintf(ipcReadInfo,"%d-%d",iIPCNtripSource,IPC_NTRIP_SOCKET_READ_SIZE8K);

		sprintf(ipcWriteInfo,"%d-%d",IPC_NTRIP_SOCKET_WRITE,IPC_NTRIP_SOCKET_WRITE_SIZE8K);
		if(process[iNtripEP0ProcessId]>0)
			release_endpoint(&endpoint,iNtripEP0ProcessId);
		//if(!creat_endpoint(&endpoint,PROCESS_NTRIP+id*2+1,0,IPC_NTRIP_SOCKET_READ+id*10+5,IPC_NTRIP_SOCKET_WRITE+id*10))
		//create source ep
		//read gnss data to ipcReadInfo
		if(!creat_endpoint(&endpoint,iNtripEP0ProcessId,0,ipcWriteInfo,ipcReadInfo))
		{
			xmlconfig->setValue("DATAFLOW."+strDataFlowId+".STATUS",STR_STATUS_NO_FREE_EP);
			return RET_UNSUPPORTED_NOW;
		}

	}else{
	//no hemishpere board
		int iIPCNtripSource=IPC_NTRIP_SOCKET_READ+id*10+5;
		int iIPCWrite=IPC_NTRIP_SOCKET_READ+id*10;
		int iNtripEP0ProcessId=PROCESS_NTRIP+id*2+1;
		char ipcWriteInfo[20];
		sprintf(ipcWriteInfo,"%d-%d",iIPCWrite,IPC_NTRIP_SOCKET_WRITE_SIZE8K);
		sprintf(ipcReadInfo,"%d-%d",iIPCNtripSource,IPC_NTRIP_SOCKET_READ_SIZE8K);

		if(process[iNtripEP0ProcessId]>0)
			release_endpoint(&endpoint,iNtripEP0ProcessId);
		//if(!creat_endpoint(&endpoint,PROCESS_NTRIP+id*2+1,0,IPC_NTRIP_SOCKET_READ+id*10+5,IPC_NTRIP_SOCKET_WRITE+id*10))
		//read gnss data to ipcReadInfo
		if(!creat_endpoint(&endpoint,iNtripEP0ProcessId,0,ipcWriteInfo,ipcReadInfo))
		{
			xmlconfig->setValue("DATAFLOW."+strDataFlowId+".STATUS",STR_STATUS_NO_FREE_EP);
			return RET_UNSUPPORTED_NOW;
		}

	}

	//start_process will restart process[PROCESS_HUB]
	if (id==0)
	{
		CheckAndStartNtripDoubleInCOM1(ipcReadInfo);
	}


    if(gpsboard==GPS_HEMISPHERE && id > 0)
    {
        sprintf(tmp,"%d",0);

        str.assign("NTRIP.CONNECTION@ID:");
        str.append(tmp);
        str.append(".DATATYPE");

        if (xmlconfig->getValue(str,&datatype)!= RET_OK)
        {
            return RET_ERR_PARAM;
        }
        transform(datatype.begin(), datatype.end(), datatype.begin(), ::toupper);

        /*sprintf(tmp,"%d",id);

        str.assign("NTRIP.CONNECTION@ID:");
        str.append(tmp);*/
		str.assign(strDataFlowId);
        str.append(".DATATYPE");
        xmlconfig->setValue(str,datatype);
    }
    else
    {

        socket_id=endpoint.id;

        //sprintf(tmp,"%d",id);
        //str.assign("NTRIP.CONNECTION@ID:");
        //str.append(tmp);
		str.assign(strDataFlowId);
        str.append(".DATATYPE");
        //cout<<"get: "<<str<<endl;
        if (xmlconfig->getValue(str,&datatype)!= RET_OK)
        {
            return RET_ERR_PARAM;
        }
        transform(datatype.begin(), datatype.end(), datatype.begin(), ::toupper);

        string value;
        int id=0;
        if (xmlconfig->getValue("SYSTEM.POSITIONING.BASESITE_ID",&value)!= RET_OK)
            value="0";
        id=atoi(value.c_str());

        oem->stopDiff(socket_id);
        if(datatype=="RTCM2")
        {
            if(!oem->issueRTCM23(socket_id,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="RTCM3")
        {
            if(!oem->issueRTCM30(socket_id,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="CMR")
        {
            if(!oem->issueCMR(socket_id,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="SCMRX")
        {
            if(!oem->issueSCMRX(socket_id,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="RTCM32")
        {
            if(!oem->issueRTCM32(socket_id,id)) return RET_EXEC_FAILED;

            string rtcmv3_eph;
            if (xmlconfig->getValue(strDataFlowId+".RTCMV3_EPH",&rtcmv3_eph)!= RET_OK)
                rtcmv3_eph="";
            oem->issueRTCMV3EPH(socket_id,rtcmv3_eph);
        }
        else if(datatype=="RAW")
        {
            if(!oem->issueRAW(socket_id,1000,-1)) return RET_EXEC_FAILED;
        }
        else  if(datatype=="DGPS")
        {
            double lat;
            double lon;
            float height;

            if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELAT",&value)!= RET_OK)
                return RET_EXEC_FAILED;
            lat=strtod(value.c_str(),NULL);
            if (xmlconfig->getValue("SYSTEM.POSITIONING.BASELON",&value)!= RET_OK)
                return RET_EXEC_FAILED;
            lon=strtod(value.c_str(),NULL);
            if (xmlconfig->getValue("SYSTEM.POSITIONING.BASEHEIGHT",&value)!= RET_OK)
                return RET_EXEC_FAILED;
            height=atof(value.c_str());

            if(!oem->issueDGPS(socket_id,lat,lon,height,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="ROX")
        {
            if(!oem->issueROX(socket_id,id)) return RET_EXEC_FAILED;
        }
        else if(datatype=="RTCM32_10")
        {
			if(!oem->issueRTCM3210(socket_id,id)) return RET_EXEC_FAILED;
        }
    }

    /////
    string ip,port,mountpoint,user,pass, userpass, addr,con_id,ntripversion;

    /*sprintf(tmp,"%d",id);
    str.assign("NTRIP.CONNECTION@ID:");
    str.append(tmp);*/
    xmlconfig->addNode("DATAFLOW."+strDataFlowId+".STATUS");
    xmlconfig->addNode("DATAFLOW."+strDataFlowId+".STARTTIME");
    xmlconfig->addNode("DATAFLOW."+strDataFlowId+".TRAFFIC");

    con_id.assign(tmp);
    if (xmlconfig->getValue(strDataFlowId+".IP",&ip)!= RET_OK)
        ip="127.0.0.1";
    if (xmlconfig->getValue(strDataFlowId+".PORT",&port)!= RET_OK)
        port="6060";
    if (xmlconfig->getValue(strDataFlowId+".MOUNTPOINT",&mountpoint)!= RET_OK)
        mountpoint="test";
    if (xmlconfig->getValue(strDataFlowId+".USER",&user)!= RET_OK)
        user="user";
    if (xmlconfig->getValue(strDataFlowId+".PASS",&pass)!= RET_OK)
        pass="pass";
    userpass=user+"|"+pass;
    if (xmlconfig->getValue(strDataFlowId+".NTRIPVERSION",&ntripversion)!= RET_OK)
        ntripversion="NTRIPV1";

    cout<< "ntrip: "<<ip<< ":"<<port<< " "<<userpass<< " "<<mountpoint<<endl;

    addr=ip+":"+port;

    //usr/bin/geo/ntrip 63.223.121.27:6060 1qaz2wsx test001 8101
    memset(exec_argv,0,sizeof(exec_argv));

    exec_argv[0] = BuildInfoFromString(NTRIP_BIN_PATH);
    exec_argv[1] = BuildInfoFromString(addr.c_str());
    exec_argv[2] = BuildInfoFromString(userpass.c_str());
    exec_argv[3] = BuildInfoFromString(mountpoint.c_str());
    //exec_argv[4] = BuildIPCInfo(IPC_NTRIP_SOCKET_WRITE+id*10,IPC_NTRIP_SOCKET_READ_SIZE8K);
	exec_argv[4] = BuildInfoFromString(ipcReadInfo);
    exec_argv[5] = BuildInfoFromString(con_id.c_str());
    exec_argv[6] = BuildInfoFromString(ntripversion.c_str());
    start_process(7,exec_argv,process[PROCESS_NTRIP+id*2]);

    xmlconfig->getValue("NTRIP.CONNECTION@ID:*.DATATYPE",&str);
    if(str.length())
        ntrip_connection=str_count(str.c_str(),"|")+1;
    else
        ntrip_connection=0;
    sprintf(tmp,"%d",ntrip_connection);
    str.assign(tmp);
    xmlconfig->setValue("NTRIP.CUR_CONNECTION",str);


    xmlconfig->setValue("DATAFLOW.NTRIP.STATUS","transmitting");
    xmlconfig->saveRealtime();

    //syslog(LOG_LOCAL7|LOG_INFO,"ntrip_connect %d\n",id);
    return RET_OK;
}

int command::set_ntrip_disconnect(int id)
{
	char tmp[128];
	string str;
	//printf("set_ntrip_disconnect %d\n",id);
	//syslog(LOG_LOCAL7|LOG_INFO,"set_ntrip_disconnect %d\n",id);

	//todo: only the last ntrip process can release ep when work with hemisphere board
	stop_process(process[PROCESS_NTRIP+id*2]);

	ENDPOINT_OPT endpoint;
	if(release_endpoint(&endpoint,PROCESS_NTRIP+id*2+1))
	{
		oem->stopDiff(endpoint.id);
	}

	string datatype;
	sprintf(tmp,"%d",id);
	str.assign("NTRIP.CONNECTION@ID:");
	str.append(tmp);
	str.append(".DATATYPE");
	//cout<<"get: "<<str<<endl;
	if (xmlconfig->getValue(str,&datatype)!= RET_OK)
	{
		return RET_ERR_PARAM;
	}

	sprintf(tmp,"%d",id);
	str.assign("NTRIP.CONNECTION@ID:");
	str.append(tmp);
	xmlconfig->setValue("DATAFLOW."+str+".STATUS","stop");
	xmlconfig->setValue("DATAFLOW."+str+".TRAFFIC","0");

	xmlconfig->getValue("NTRIP.CONNECTION@ID:*.DATATYPE",&str);
	if(str.length())
		ntrip_connection=str_count(str.c_str(),"|")+1;
	else
		ntrip_connection=0;
	sprintf(tmp,"%d",ntrip_connection);
	str.assign(tmp);
	xmlconfig->setValue("NTRIP.CUR_CONNECTION",str);
	if(ntrip_connection==0)stop_process(process[PROCESS_HUB]);

	xmlconfig->getValue("DATAFLOW.NTRIP.CONNECTION@ID:*.STATUS",&str);
	if(!strstr(str.c_str(),"transmitting"))
	{
		xmlconfig->setValue("DATAFLOW.NTRIP.STATUS","stop");
		if(!checkPortsRTCMOut())
		{
			dev_StopBase();
		}
	}

	xmlconfig->saveRealtime();

	//syslog(LOG_LOCAL7|LOG_INFO,"ntrip_disconnect %d\n",id);
	return RET_OK;
}

int command::Network()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_PRIORITY:
        return net_priority();
    case CMD_WAN:
        return net_wan();
    case CMD_WIFI:
        return net_wifi();
    case CMD_GPRS:
        return net_gprs();
    case CMD_DNS1:
        return net_dns1();
    case CMD_FTP:
        return net_ftp();
    case CMD_SNAT:
        return net_snat();
    case CMD_DNAT:
        return net_dnat();
	case CMD_NTP:
		return net_ntp();
	case CMD_DYNAMIC_DNS:
		return net_DynamicDns();
    default:
        return -1;
    }
    return -1;
}

int command::net_snat()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_snat_Reset();
    default:
        return -1;
    }
    return 0;
}

int command::net_snat_Reset()
{
    char tmpStr[256],number[24];;
    string cmd,value,tmpValue;
    string snat_ip, dest_dev;
    FILE *fs=NULL;
    int i,n;

    if (xmlconfig->getValue("NETWORK.SNAT.TOTEL_LIST",&value)!= RET_OK)
        return RET_ERR_PARAM;

    n=atoi(value.c_str());
    for(i=0; i<n; i++)
    {
        cmd.clear();
        value.clear();
        tmpValue.clear();
        snat_ip.clear();
        dest_dev.clear();

        cmd="iptables -t nat -A POSTROUTING -s ";
        sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.ENABLE",i);
        if (xmlconfig->getValue(tmpStr,&value)== RET_OK)
        {
            //iptables -t nat -A POSTROUTING -s 192.168.10.0/24 -o eth0 -j MASQUERADE
            sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.SNAT_IP",i);
            xmlconfig->getValue(tmpStr,&snat_ip);

            sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.DEST_DEV",i);
            xmlconfig->getValue(tmpStr,&dest_dev);

            if(snat_ip.empty()||dest_dev.empty())
            {
                printf("empty\n");
                continue;
            }

            sprintf(tmpStr,"iptables -t nat -vnL --line-numbers | grep MASQUERADE | grep %s | grep %s | awk '{print $1}'",
                    snat_ip.c_str(), dest_dev.c_str());
            //printf("tmpStr: %s\n", tmpStr);



            //先判断是否存在这个规则
            fs=popen(tmpStr,"r");
            memset(number, 0, sizeof(number));
            if( NULL!=fgets(number, sizeof(number), fs) )
            {
                printf("snat get number %s\n", number);
                if(value.compare("YES") == 0)
                {
                    pclose(fs);
                    continue;                    //已存在
                }
                else
                {
                    cmd = "iptables -t nat -D POSTROUTING ";
                    string s(&number[0],&number[strlen(number)]);
                    //printf("s : %s\n", s.c_str());
                    cmd = cmd + s;
                }
            }
            else
            {
                if(value.compare("YES") == 0)
                {
                    cmd = cmd + snat_ip + " -o " + dest_dev + " -j MASQUERADE";
                }
                else
                {
                    pclose(fs);
                    continue;
                }
            }

            pclose(fs);
            printf("cmd : %s\n", cmd.c_str());

            syslog(LOG_LOCAL7|LOG_INFO,"cmd:  %s\n",cmd.c_str());

            system(cmd.c_str());
        }
    }
    return 0;
}

int command::net_dnat()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_dnat_Reset();
    default:
        return -1;
    }
    return 0;
}


int command::net_dnat_Reset()
{
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Reset()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
	    char tmpStr[512],number[16];
        string cmd,value,tmpValue;
        string source_dev,source_ip,source_port,dest_ip,dest_port;
        FILE *fs=NULL;
        int i,n;

        if (xmlconfig->getValue("NETWORK.DNAT.TOTEL_LIST",&value)!= RET_OK)
		{
			exit(1);
            //return RET_ERR_PARAM;
		}
        n=atoi(value.c_str());
        for(i=0; i<n; i++)
        {
            cmd.clear();
            source_dev.clear();
            source_ip.clear();
            source_port.clear();
            dest_ip.clear();
            dest_port.clear();

            sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.ENABLE",i);
            if (xmlconfig->getValue(tmpStr,&value)== RET_OK)
            {
                sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_NETDEV",i);
                xmlconfig->getValue(tmpStr,&source_dev);

                sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_IP",i);
                xmlconfig->getValue(tmpStr,&source_ip);

                sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_PORT",i);
                xmlconfig->getValue(tmpStr,&source_port);

                sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.DEST_IP",i);
                xmlconfig->getValue(tmpStr,&dest_ip);

                sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.DEST_PORT",i);
                xmlconfig->getValue(tmpStr,&dest_port);

                if(source_dev.empty()||source_ip.empty()||source_port.empty()||dest_ip.empty()||dest_port.empty())
                {
                    printf("empty\n");
                    continue;
                }
                sprintf(tmpStr,"iptables -t nat -vnL --line-numbers | grep %s | grep %s | grep dpt:%s | grep to:%s:%s | awk '{print $1}'",
                    source_dev.c_str(), source_ip.c_str(), source_port.c_str(), dest_ip.c_str(), dest_port.c_str());

                //先判断是否存在这个规则
                fs=popen(tmpStr,"r");
                memset(number, 0, sizeof(number));
                if( NULL!=fgets(number, sizeof(number), fs) )
                {
                    printf("dnat get number %s\n", number);
                    if(value.compare("YES") == 0)
                    {
                        // printf("1 ===============\n");
                        pclose(fs);
                        continue;                    //已存在
                    }
                    else
                    {
                        //  printf("2 ===============\n");
                        cmd = "iptables -t nat -D PREROUTING ";
                        string s(&number[0],&number[strlen(number)]);
                        cmd = cmd + s;
                    }
                }
                else
                {
                    if(value.compare("YES") == 0)
                    {
                   // printf("3 ===============\n");
                //                                         source_dev     source_ip           source_port    dest_ip:dest_port
                // iptables -t nat -vnL --line-numbers |grep wlan0 |grep 192.168.20.2 | grep dpt:81 |grep to:192.168.30.2:80
                        cmd = "iptables -t nat -A PREROUTING -i ";
                        cmd = cmd + source_dev + " -d " + source_ip + " -p  tcp --dport " + source_port + " -j DNAT --to-destination " + dest_ip + ":" + dest_port;
                    }
                    else
                    {
                        pclose(fs);
                        continue;
                    }
                }
                pclose(fs);
                system(cmd.c_str());
            }
        }
        exit(0);
    }
    return 0;
}

int command::net_dns1()
{
    enPathField enpath =(enPathField)node[2];
    switch(enpath)
    {
    case CMD_RESET:
        return net_dns1_Reset();
    default:
        return -1;
    }
    return -1;
}

int command::net_dns1_Reset()
{
    char tmp[50];
    char iptmp[16];
    char *resolvbuf=NULL, *ip=NULL, *p1=NULL, *p2=NULL;
    int len=0, maxlen=0;
    FILE *fp = NULL;
    string value;
	int iRet=-1;
    xmlconfig->getValue("NETWORK.DNS1.SELECT",&value);
    if(strcmp(value.c_str(),"Custom")==0)
        xmlconfig->getValue("NETWORK.DNS1.CUSTOM",&value);

    len=value.length();
    ip=(char*)calloc(len+2, sizeof(char));
    if(ip==NULL)
        return -1;
    strncpy(ip, value.c_str(), len);

    maxlen=((len/7)+1)*28;
    resolvbuf=(char*)calloc(maxlen+2, sizeof(char));
    if(resolvbuf==NULL)
        return -1;

    p1=ip;
    while(1)
    {
        memset(iptmp, 0, sizeof(iptmp));
        p2=strchr(p1, '|');
        if(p2==NULL)
        {
            strncpy(iptmp, p1, strlen(p1));
            sprintf(tmp, "%s%s%s", "nameserver ", iptmp, "\r\n");
            strcat(resolvbuf, tmp);
            break;
        }
        else
        {
            strncpy(iptmp, p1, (p2-p1));
            sprintf(tmp, "%s%s%s", "nameserver ", iptmp, "\r\n");
            strcat(resolvbuf, tmp);
            p1=p2+1;
            p2=ip;
            if((p1-p2)>=len)
            {
                printf("p1=NULL\n");
                break;
            }
        }
    }

    fp = fopen("/etc/resolv.conf", "w+");   //
    if( NULL!=fp )
    {
        fseek(fp, 0, SEEK_SET);
        fwrite(resolvbuf, 1, strlen(resolvbuf), fp);
        fflush(fp);
        fclose(fp);
        iRet= RET_OK;
    }
	free(ip);
	free(resolvbuf);
    return iRet;
}

int command::net_priority()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_priority_Reset();
    default:
        return -1;
    }
    return -1;
}

int command::net_priority_Reset()
{
    string cmd,value;
    cmd.assign("/geo/app/sendmsg ");
    xmlconfig->getValue("NETWORK.PRIORITY",&value);
    transform(value.begin(), value.end(), value.begin(), ::toupper);

    cmd=cmd+"10L "+"h "+value;

    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_priority()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        if(system(cmd.c_str())==-1)
        {
            //syslog(LOG_LOCAL7|LOG_ERR,"net_priority()  system() error!");
            ;
        }
        exit(0);
    }
    return RET_OK;
}

#define VPN_SERVER_ROUTE "/geo/app/ceitsa_vpn/route.up"
#define VPN_CLIENT_CONF "/geo/app/ceitsa_vpn/ceitsa.conf"
#define PROXY_NARI_CONF "/geo/app/nari_proxy/config"
int command::Vpn()
{
	enPathField enpath =(enPathField)node[1];
	//printf("Record %d \n", enpath) ;
	switch(enpath)
	{
	case CMD_RESET:
		return vpn_Reset();
    case CMD_NARI:
		return vpn_Nari();
	default:
		return -1;
	}
	return -1;
}

int command::vpn_Nari()
{
    enPathField enpath =(enPathField)node[2];
	//printf("Record %d \n", enpath) ;
	switch(enpath)
	{
	case CMD_RESET:
		return vpn_NariReset();
	default:
		return -1;
	}
	return -1;
}

int command::vpn_NariReset()
{
    string remote1,remote2;

    xmlconfig->getValue("VPN.NARI.REMOTE1",&remote1);
    xmlconfig->getValue("VPN.NARI.REMOTE2",&remote2);


    char cmd[100];
    //使用#来代替/当分隔符，因此/不再具有转义功能，无需再加反斜杠来辨识
    //sed -i "s/^\[REMOTE1\]=.*/[REMOTE1]=58.62.206.155:6075/g" /geo/app/nari_proxy/config
    sprintf(cmd,"sed -i \"s#^\\[REMOTE1\\]=.*#[REMOTE1]=%s#g\" %s",remote1.c_str(),PROXY_NARI_CONF);
    system(cmd);

    sprintf(cmd,"sed -i \"s#^\\[REMOTE2\\]=.*#[REMOTE2]=%s#g\" %s",remote2.c_str(),PROXY_NARI_CONF);
    system(cmd);

    system("kill `ps | grep -v grep | grep nari_proxy | awk '{print $1}'` 1> /dev/null 2>&1");

    return RET_OK;
}

int command::vpn_Reset()
{
    string remote,route;
	string addr,port;

	string platform;
	char cmd[100];

	if(xmlconfig->getValue("VPN.PLATFORM",&platform)!=RET_OK || platform.length()==0){
        platform="0";
	}

	system("echo 0 > /geo/app/VPN_platform");
	system("kill `ps | grep -v grep | grep ceitsa | awk '{print $1}'` 1> /dev/null 2>&1");
	system("kill `ps | grep -v grep | grep nari_proxy | awk '{print $1}'` 1> /dev/null 2>&1");

	if(platform=="1")//普华
    {
        xmlconfig->getValue("VPN.REMOTE",&remote);
        xmlconfig->getValue("VPN.ROUTE",&route);

        size_t pos;
        if((pos=remote.find(":"))!=std::string::npos)
        {
            addr = remote.substr(0,pos);
            port = remote.substr(pos+1,remote.length());
        }
        else
            return RET_ERR_PARAM;

        char remote_s[128];

        sprintf(remote_s,"remote %s %s",addr.c_str(),port.c_str());

        sprintf(cmd,"sed -i \"s#`grep -i ^remote %s`#%s#g\" %s",VPN_CLIENT_CONF,remote_s,VPN_CLIENT_CONF);
        system(cmd);

        sprintf(cmd,"sed -i \"s#`grep -i ^route %s`#route add -net %s dev tun0#g\" %s",VPN_SERVER_ROUTE,route.c_str(),VPN_SERVER_ROUTE);
        system(cmd);
    }
	else if(platform=="2")//南瑞
    {
        string remote1,remote2;

        xmlconfig->getValue("VPN.NARI.REMOTE1",&remote1);
        xmlconfig->getValue("VPN.NARI.REMOTE2",&remote2);

        //使用#来代替/当分隔符，因此/不再具有转义功能，无需再加反斜杠来辨识
        //sed -i "s/^\[REMOTE1\]=.*/[REMOTE1]=58.62.206.155:6075/g" /geo/app/nari_proxy/config
        sprintf(cmd,"sed -i \"s#^\\[REMOTE1\\]=.*#[REMOTE1]=%s#g\" %s",remote1.c_str(),PROXY_NARI_CONF);
        system(cmd);

        sprintf(cmd,"sed -i \"s#^\\[REMOTE2\\]=.*#[REMOTE2]=%s#g\" %s",remote2.c_str(),PROXY_NARI_CONF);
        system(cmd);
    }

	sprintf(cmd,"echo \"%s\" > /geo/app/VPN_platform",platform.c_str());
    system(cmd);

#if 0
	xmlconfig->getValue("VPN.REMOTE",&remote);
	xmlconfig->getValue("VPN.ROUTE",&route);

    size_t pos;
	if((pos=remote.find(":"))!=std::string::npos)
	{
		addr = remote.substr(0,pos);
		port = remote.substr(pos+1,remote.length());
	}
	else
		return RET_ERR_PARAM;

    char cmd[100];
    char remote_s[128];

	sprintf(remote_s,"remote %s %s",addr.c_str(),port.c_str());

	sprintf(cmd,"sed -i \"s#`grep -i ^remote %s`#%s#g\" %s",VPN_CLIENT_CONF,remote_s,VPN_CLIENT_CONF);
    system(cmd);


	sprintf(cmd,"sed -i \"s#`grep -i ^route %s`#route add -net %s dev tun0#g\" %s",VPN_SERVER_ROUTE,route.c_str(),VPN_SERVER_ROUTE);
    system(cmd);


	system("kill `ps | grep -v grep | grep ceitsa | awk '{print $1}'` 1> /dev/null 2>&1");
	//usleep(200*1000);
#endif
	//system("/geo/app/ceitsa_vpn/ceitsa --config /geo/app/ceitsa_vpn/ceitsa.conf --daemon");
	//system("/geo/app/scripts/start_vpn.sh");
	/*FILE * fp;
	char buffer[80];
	fp=popen("/geo/app/scripts/start_vpn.sh","r");
	fgets(buffer,sizeof(buffer),fp);
	fprintf(stdout,"%s",buffer);
	pclose(fp);*/

	return RET_OK;
}

int command::net_ntp()
{
	enPathField enpath =(enPathField)node[2];
	//printf("Record %d \n", enpath) ;
	switch(enpath)
	{
	case CMD_RESET:
		return net_ntp_Reset();
	default:
		return -1;
	}
	return -1;
}

int command::net_ntp_Reset()
{
	string enable;
	char cmd[256]={0};

	if(!ntp_enable){
		syslog(LOG_LOCAL7|LOG_INFO,"NTP Not ready .");
		return RET_INVALID_OPERATION;
	}

	xmlconfig->getValue("NETWORK.NTP.ENABLE",&enable);
	if(enable.length()==0) enable="NO";

	if(enable.compare("YES")==0)
	{
		pid_t pid;
		pid=fork();
		if (pid == -1)
		{
			syslog(LOG_LOCAL7|LOG_ERR,"net_ntp_Reset()  fork() error!");
			return RET_EXEC_FAILED;
		}
		if (pid == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"/usr/sbin/ntpd -c %s","/etc/ntp.conf");
			system("kill `ps | grep -v grep | grep ntpd | awk '{print $1}'` 1> /dev/null 2>&1");
			sleep(2);

			syslog(LOG_LOCAL7|LOG_INFO,"==========net_ntp_Reset==============");
			system(cmd);
			exit(0);
		}
	}
	else
	{
		system("kill `ps | grep -v grep | grep ntpd | awk '{print $1}'` 1> /dev/null 2>&1");
	}

	return RET_OK;
}

int command::net_ftp()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_ftp_Reset();
    default:
        return -1;
    }
    return -1;
}

int command::net_ftp_Reset()
{
    string cmd,file;
    string anonymous,user,pass;
    xmlconfig->getValue("NETWORK.FTP.ENABLE_ANONYMOUS",&anonymous);
    if(anonymous.length()==0) anonymous="NO";

    system("mkdir -p /etc/ftp_user/");
    system("rm /etc/ftp_user/*");

    transform(anonymous.begin(), anonymous.end(), anonymous.begin(), ::toupper);
    if(anonymous.compare("YES") == 0)
    {
        system("touch /etc/ftp_user/anonymous");
        system("echo \"123456\" > /etc/ftp_user/anonymous");
        system("sed -i \"s/`grep -i ^anonymous_enable /etc/vsftpd.conf`/anonymous_enable=YES/g\" /etc/vsftpd.conf");
    }
    else
    {
        system("sed -i \"s/`grep -i ^anonymous_enable /etc/vsftpd.conf`/anonymous_enable=NO/g\" /etc/vsftpd.conf");
    }
    xmlconfig->getValue("NETWORK.FTP.USER",&user);
    if(user.length()==0) user="admin";
    xmlconfig->getValue("NETWORK.FTP.PASS",&pass);
    if(pass.length()==0) pass="password";


    file="/etc/ftp_user/"+user;
    cmd="touch "+file;
    system(cmd.c_str());

    cmd="echo \""+pass+"\" > "+file;
    system(cmd.c_str());

    system("killall vsftpd 1> /dev/null 2>&1");
    system("/usr/sbin/vsftpd &");

    return RET_OK;
}

int command::net_DynamicDns()
{
	enPathField enpath =(enPathField)node[2];
	//printf("Record %d \n", enpath) ;
	switch(enpath)
	{
	case CMD_ENABLE:
		return net_DynDns_Enable();
	default:
		return -1;
	}
	return -1;
}

int command::net_DynDns_Enable()
{
	string value;
	transform(param.begin(), param.end(), param.begin(), ::toupper);

	/*string server,host,user,pass;
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.SERVICE_PROVIDER",&server);
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.HOST",&host);
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.USER",&user);
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.PASS",&pass);*/
	//setting=server+"_"+host +"_"+user+"_"+pass;

	xmlconfig->setValue("NETWORK.DYNAMIC_DNS.ENABLE",param);
	xmlconfig->saveConfig();

	system("killall ddns.sh");

	net_set_ddns();

	return RET_OK;
}

int command::net_set_ddns()
{
	string value;
	string cmd,server,host,user,pass;

	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.ENABLE",&value);
	transform(value.begin(), value.end(), value.begin(), ::toupper);
	if(value.compare("YES") != 0)return RET_OK;

	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.SERVICE_PROVIDER",&value);
	if(value.length()==0)
	{
		value="dyndns.com";
		xmlconfig->setValue("NETWORK.DYNAMIC_DNS.SERVICE_PROVIDER",value);
	}
	if(value.find("dyndns")!=std::string::npos || value.find("DYNDNS")!=std::string::npos) server="dyndns";
	else if(value.find("no-ip")!=std::string::npos || value.find("NO-IP")!=std::string::npos) server="no-ip";
	else if(value.find("easydns")!=std::string::npos || value.find("EASYDNS")!=std::string::npos) server="easydns";
	else if(value.find("two-dns")!=std::string::npos || value.find("TWO-DNS")!=std::string::npos|| value.find("TWODNS")!=std::string::npos) server="twodns";
	else if(value.find("oray")!=std::string::npos || value.find("ORAY")!=std::string::npos) server="oray";
	else return RET_EXEC_FAILED;

	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.HOST",&host);
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.USER",&user);
	xmlconfig->getValue("NETWORK.DYNAMIC_DNS.PASS",&pass);
	//ddns_setting=value+"_"+host +"_"+user+"_"+pass;

	cmd.assign("/geo/app/scripts/ddns.sh ");
	cmd=cmd+ server+" "+host +" "+user+" "+pass+"  &";

	system("killall ddns.sh");
	syslog(LOG_LOCAL7|LOG_INFO,"DDNS: %s\n",cmd.c_str());
	system(cmd.c_str());

	return RET_OK;
}

int command::net_wan_Reset()
{
    #if 1
    string  mac,dhcp,ip, mask,gateway;
    string cmd;

    /*
        cmd.assign("/sbin/ifconfig eth0 down");
        system(cmd.c_str());

        if (xmlconfig->getValue("DEVICE.INFO.MAC",&mac)!= RET_OK || mac.length()==0)
        {
            mac="00:11:D8:76:24:E0";
        }
        cmd.assign(SETMAC_PATH);
        cmd=cmd+" "+mac;
        cout<< cmd<<endl;
        system(cmd.c_str());

        cmd.assign("/sbin/ifconfig eth0 up");
        system(cmd.c_str());
    */
    if (xmlconfig->getValue("NETWORK.WAN.DHCP",&dhcp)!= RET_OK)
    {
        //cout<< "read value error."<<endl;
        syslog(LOG_LOCAL7|LOG_ERR,"cmd read dhcp value error!");
        return RET_EXEC_FAILED;
    }
    //cout<< "dhcp "<< dhcp<<endl;

    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"cmd wan reset fork() error!");
        return false;
    }
    if (pid == 0)
    {
        if(dhcp=="1"||dhcp=="YES")
        {
            system("/geo/app/sendmsg 10L a");
        }
        else
        {
            char str[128];
            if (xmlconfig->getValue("NETWORK.WAN.IP",&ip)!= RET_OK)
            {
                syslog(LOG_LOCAL7|LOG_ERR,"cmd read ip value error!");
                exit(RET_EXEC_FAILED);
            }

            if (xmlconfig->getValue("NETWORK.WAN.MASK",&mask)!= RET_OK)
            {
                //cout<< "read mask error."<<endl;
                syslog(LOG_LOCAL7|LOG_ERR,"cmd read mask value error!");
                exit(RET_EXEC_FAILED);
            }

            if (xmlconfig->getValue("NETWORK.WAN.GATEWAY",&gateway)!= RET_OK)
            {
                //cout<< "read gateway error."<<endl;
                syslog(LOG_LOCAL7|LOG_ERR,"cmd read gateway value error!");
                exit(RET_EXEC_FAILED);
            }

            sprintf(str,"/geo/app/sendmsg 10L b %s %s %s",ip.c_str(), mask.c_str(), gateway.c_str());
            system(str);
        }
        exit(0);
    }

#endif
    return RET_OK;
}

int command::net_wan_Connected()
{
    if(param.empty())
    {
        return RET_ERR_PARAM;
    }
    transform(param.begin(), param.end(), param.begin(), ::toupper);
    //xmlconfig->setValue("NETWORK.WAN.CONNECTED",param);

    if(param.compare("1") == 0||param.compare("YES") == 0 )//wan connect
    {
        return net_wan_Reset();
    }
    else
    {
        //xmlconfig->setValue("DATAFLOW.NTRIP.STATUS","idle");
        xmlconfig->saveRealtime();
    }

    return RET_OK;
}

int command::net_wan_IP()
{
	xmlconfig->setValue("NETWORK.WAN.IP",param);

	if(is_validIPv4Address(param.c_str())>0)
	{
		net_set_ddns();
	}

	return RET_OK;
}

int command::net_wan()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_wan_Reset();
    case CMD_CONNECT:
        return net_wan_Connected();
	case CMD_IP:
		return net_wan_IP();
    default:
        return -1;
    }
    return -1;
}


int command::net_wifi()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_wifi_Reset();
    case CMD_CONNECT:
        return net_wifi_Connect();
    case CMD_SCAN:
        return net_wifi_Scan();
	case CMD_IP:
		return net_wifi_IP();
    default:
        return -1;
    }
    return -1;
}

int command::net_wifi_Reset()
{
    string dhcp,value,mode;
    string cmd;
    cmd.assign("/geo/app/sendmsg ");
    //cmd2.assign("");
    xmlconfig->getValue("NETWORK.WIFI.ENABLE",&value);
    transform(value.begin(), value.end(), value.begin(), ::toupper);
    if(value.compare("YES") == 0)
    {
        //xmlconfig->getValue("NETWORK.WIFI.AP_MODE",&mode);
        xmlconfig->getValue("NETWORK.WIFI.WIFI_MODE",&mode);
        transform(mode.begin(), mode.end(), mode.begin(), ::toupper);//字母转大写
        if(mode.compare("AP") == 0)
        {
            string ap_ip,ap_pass,ap_ssid;
            ap_ip.clear();
            ap_pass.clear();
            ap_ssid.clear();

            if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&ap_ssid)!= RET_OK || ap_ssid.length()==0)
                ap_ssid="SC200";
            if (xmlconfig->getValue("NETWORK.WIFI.AP_PASS",&ap_pass)!= RET_OK)
                ap_pass="NONE";
            if (xmlconfig->getValue("NETWORK.WIFI.AP_IP",&ap_ip)!= RET_OK)
                ap_ip="192.168.10.1";
            cmd=cmd+"10L "+"d "+"ENABLE AP "+ap_ssid+" "+ap_pass+" "+ap_ip;
        }
        else if(mode.compare("CLIENT") == 0)
        {
            string lang;
            string virtualap;
            lang.clear();
            virtualap.clear();
            xmlconfig->getValue("SYSTEM.LANG",&lang);
            xmlconfig->getValue("NETWORK.WIFI.VIRTUALAP",&virtualap);

            if (xmlconfig->getValue("NETWORK.WIFI.DHCP",&dhcp)!= RET_OK)
            {
                //cout<< "read value error."<<endl;
                syslog(LOG_LOCAL7|LOG_ERR,"cmd read dhcp value error!");
                return RET_EXEC_FAILED;
            }
                //cout<< "dhcp "<< dhcp<<endl;
            if(dhcp=="1"||dhcp=="YES")
            {
                string ssid,pass;
                ssid.clear();
                pass.clear();
                xmlconfig->getValue("NETWORK.WIFI.ROUTER",&ssid);
                xmlconfig->getValue("NETWORK.WIFI.PASS",&pass);

                if((lang.compare("ru") == 0)&&(pass.compare("HET")==0))
                    pass.assign("NONE");
                else if(pass.length()==0)
                    pass.assign(" ");

                if(ssid.length())
                {
                    if(virtualap.compare("OFF")==0)
                        cmd=cmd+"10L "+"c1 "+ "0" +" "+ssid+" "+pass;
                    else
                    {
                        string ap_ip, ap_pass, ap_ssid;
                        ap_ip.clear();
                        ap_pass.clear();
                        ap_ssid.clear();
                        if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&ap_ssid)!= RET_OK || ap_ssid.length()==0)
                            ap_ssid="SC200";
                        if (xmlconfig->getValue("NETWORK.WIFI.AP_IP",&ap_ip)!= RET_OK)
                            ap_ip="192.168.10.1";
                        if (xmlconfig->getValue("NETWORK.WIFI.AP_PASS",&ap_pass)!= RET_OK)
                            ap_pass="NONE";

                        if((lang.compare("ru") == 0)&&(ap_pass.compare("HET")==0))
                            ap_pass.assign("NONE");
                        else if(ap_pass.length()==0)
                            ap_pass.assign(" ");

                        //cmd=cmd+"10L "+"c1 "+ "1" +" "+ssid+" "+pass+" "+ap_ssid+" "+ap_ip+" "+ap_pass;
                         cmd=cmd+"10L "+"c1 "+ "1" +" "+ ssid+" "+pass+" "+ "0"+" "+"0" +" "+"0"+" "+ap_ssid+" "+ap_ip+ " "+ap_pass;
                    }
                }
            }
            else
            {
                string client_ssid, client_pass, client_ip, client_mask, client_gateway;
                client_ssid.clear();
                client_pass.clear();
                client_ip.clear();
                client_mask.clear();
                client_gateway.clear();
                xmlconfig->getValue("NETWORK.WIFI.ROUTER",&client_ssid);
                xmlconfig->getValue("NETWORK.WIFI.PASS",&client_pass);
                xmlconfig->getValue("NETWORK.WIFI.IP",&client_ip);
                xmlconfig->getValue("NETWORK.WIFI.MASK",&client_mask);
                xmlconfig->getValue("NETWORK.WIFI.GATEWAY",&client_gateway);
                if((lang.compare("ru") == 0)&&(client_pass.compare("HET")==0))
                    client_pass.assign("NONE");
                else if(client_pass.length()==0)
                    client_pass.assign(" ");
                if(client_ssid.length())
                {
                    if(virtualap.compare("OFF")==0)
                        cmd=cmd+"10L "+"c0 "+ "0" +" "+client_ssid+" "+client_pass+" "+client_ip+" "+client_mask+" "+client_gateway;
                    else
                    {
                        string ap_ip, ap_pass, ap_ssid;
                        ap_ip.clear();
                        ap_pass.clear();
                        ap_ssid.clear();
                        if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&ap_ssid)!= RET_OK || ap_ssid.length()==0)
                            ap_ssid="SC200";
                        if (xmlconfig->getValue("NETWORK.WIFI.AP_IP",&ap_ip)!=RET_OK)
                            ap_ip="192.168.10.1";
                        if (xmlconfig->getValue("NETWORK.WIFI.AP_PASS",&ap_pass)!=RET_OK)
                            ap_pass="NONE";

                        if((lang.compare("ru") == 0)&&(ap_pass.compare("HET")==0))
                            ap_pass.assign("NONE");
                        else if(ap_pass.length()==0)
                            ap_pass.assign(" ");
                        cmd=cmd+"10L "+"c0 "+ "1" +" "+client_ssid+" "+client_pass+" "+client_ip+" "+client_mask+" "+client_gateway
                                                  +" "+ap_ssid+" "+ap_ip+ " "+ap_pass;
                    }
                }
            }
        }
        else if(mode.compare("MESH") == 0)
        {
            string lang;
            lang.clear();
            xmlconfig->getValue("SYSTEM.LANG",&lang);

            string ser_cli,m_cli_name,m_cli_ip,m_dhcp,m_channel,m_mask,m_gateway;
            string work_mode;
            ser_cli.clear();
            m_dhcp.clear();
            m_cli_name.clear();
            m_cli_ip.clear();
            m_channel.clear();
            m_mask.clear();
            m_gateway.clear();

            if (xmlconfig->getValue("NETWORK.WIFI.MESHMODE",&ser_cli)!= RET_OK)
                ser_cli="0";
            if (xmlconfig->getValue("NETWORK.WIFI.DHCP",&m_dhcp)!= RET_OK)
                m_dhcp="0";

            if (xmlconfig->getValue("NETWORK.WIFI.MESHNAME",&m_cli_name)!=RET_OK || m_cli_name.length()==0)
                m_cli_name="NSC200_MESH";
            if (xmlconfig->getValue("NETWORK.WIFI.MESHCHANNEL",&m_channel)!=RET_OK || m_channel.length()==0)
                m_channel="1";
            if (xmlconfig->getValue("NETWORK.WIFI.IP",&m_cli_ip)!=RET_OK || m_cli_ip.length()==0)
                m_cli_ip="192.168.2.1";
            if (xmlconfig->getValue("NETWORK.WIFI.MASK",&m_mask)!=RET_OK || m_mask.length()==0)
                m_mask="255.255.255.0";
            if (xmlconfig->getValue("NETWORK.WIFI.GATEWAY",&m_gateway)!=RET_OK || m_gateway.length()==0)
                m_gateway="192.168.2.1";

            cmd.clear();
            cmd = "/geo/app/sendmsg ";

            //if(ser_cli == "1" && m_dhcp == "1") work_mode = "mode1";
            //if(ser_cli == "1" && m_dhcp == "0") work_mode = "mode2";
            //if(ser_cli == "0" && m_dhcp == "1") work_mode = "mode3";
            //if(ser_cli == "0" && m_dhcp == "0") work_mode = "mode4";

            if(m_cli_name.length())
            {
                // /geo/app/sendmsg 10L M1 0 NSC200_MESH 1 192.168.2.1 255.255.255.0 192.168.2.1
                cmd=cmd+"10L "+"m"+ser_cli+" "+m_dhcp+" "+m_cli_name+" "+m_channel+" "+m_cli_ip+" "+m_mask+" "+m_gateway;
                syslog(LOG_LOCAL7|LOG_INFO,"mode:meshap!send msg if ok!:%s\n",cmd.c_str());
            }
        }
        wifi_enable=1;
    }
    else
    {
        cmd=cmd+"10L "+"e "+"DISABLE";
        if(!wifi_enable)
            return RET_OK;
        wifi_enable=0;
    }

    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Reset()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        system(cmd.c_str());
        exit(0);
    }

    return RET_OK;
}
#if 0
int command::net_wifi_Reset()
{
    string value,mode,name,dhcp;
    string cmd;
    string cmd2;
    int connect=0;
    cmd.assign("/usr/bin/geo/scripts/wifi.sh ");
    cmd2.assign("");
    xmlconfig->getValue("NETWORK.WIFI.ENABLE",&value);
    transform(value.begin(), value.end(), value.begin(), ::toupper);
    if(value.compare("YES") == 0)
    {
        xmlconfig->getValue("NETWORK.WIFI.AP_MODE",&mode);
        transform(mode.begin(), mode.end(), mode.begin(), ::toupper);
        if(mode.compare("YES") == 0)
        {
            string ap_ip,ap_pass;
            if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&name)!= RET_OK || name.length()==0)
                name="UR380";

            xmlconfig->getValue("DEVICE.INFO.MODEL",&value);
            if(value.find("380") != value.npos)
                name="UR380_UNICORE_"+name.substr(name.length()-4,4);

            if (xmlconfig->getValue("NETWORK.WIFI.AP_PASS",&ap_pass)!= RET_OK||ap_pass.length()==0)
                ap_pass="NONE";
            if (xmlconfig->getValue("NETWORK.WIFI.AP_IP",&ap_ip)!= RET_OK||ap_ip.length()==0)
                ap_ip="192.168.10.1";

            cmd=cmd+"ENABLE AP "+name+" "+ap_pass+" "+ap_ip;
        }
        else
        {
            if (xmlconfig->getValue("NETWORK.WIFI.DHCP",&dhcp)!= RET_OK)
            {
                //cout<< "read value error."<<endl;
                syslog(LOG_LOCAL7|LOG_ERR,"cmd read dhcp value error!");
                return RET_EXEC_FAILED;
            }
            //cout<< "dhcp "<< dhcp<<endl;
            if(dhcp=="1"||dhcp=="YES")
            {
                string ssid,pass;
                ssid.clear();
                pass.clear();
                xmlconfig->getValue("NETWORK.WIFI.ROUTER",&ssid);
                xmlconfig->getValue("NETWORK.WIFI.PASS",&pass);
                if(pass.length()==0) pass.assign("NONE");
                if(ssid.length())
                {
                    connect=1;
                    cmd2.assign("/usr/bin/geo/scripts/wifi_connect.sh ");
                    cmd2=cmd2+"CONNECT "+ssid+" "+pass;
                }
                cmd=cmd+"ENABLE NO";
            }
            else
            {
                string ssid,pass,ip,mask,gateway;
                ssid.clear();
                pass.clear();
                ip.clear();
                xmlconfig->getValue("NETWORK.WIFI.ROUTER",&ssid);
                xmlconfig->getValue("NETWORK.WIFI.PASS",&pass);
                xmlconfig->getValue("NETWORK.WIFI.IP",&ip);
                xmlconfig->getValue("NETWORK.WIFI.MASK",&mask);
                xmlconfig->getValue("NETWORK.WIFI.GATEWAY",&gateway);
                if(pass.length()==0) pass.assign("NONE");
                if(ssid.length())
                {
                    connect=1;
                    cmd2.assign("/usr/bin/geo/scripts/wifi_connect.sh ");
                    cmd2=cmd2+"CONNECT "+ssid+" "+pass+" "+ip+" "+mask+" "+gateway;
                }
                cmd=cmd+"ENABLE NO";
            }
        }

        //if(wifi_enable) return RET_OK;
        wifi_enable=1;
    }
    else
    {
        cmd=cmd+"DISABLE";
        if(!wifi_enable) return RET_OK;
        wifi_enable=0;
    }

    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Reset()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        if(mode.compare("YES") == 0 || value.compare("NO") == 0)
        {
            if(system(cmd.c_str())==-1)
            {
                //syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Reset()  system() error!");
            }
        }
        if(connect) system(cmd2.c_str());
        //cout<<cmd<<endl;
        exit(0);
    }

    return RET_OK;
}
#endif

int command::net_wifi_Connect()
{
    /*
    int res;
    string value;
    string cmd;
    string ssid,pass;
    ssid.clear();
    pass.clear();
    xmlconfig->getValue("NETWORK.WIFI.ROUTER",&ssid);
    xmlconfig->getValue("NETWORK.WIFI.PASS",&pass);
    if(pass.length()==0) pass.assign("NONE");
    if(ssid.length()==0)
    {
        return RET_ERR_PARAM;
    }
    cmd.assign("/usr/bin/geo/scripts/wifi_connect.sh ");

    printf("#####################\n");
    printf("#####################\n");
    printf("#####################\n");
    printf("#####################\n");

    if(!wifi_enable)
    {
        xmlconfig->setValue("NETWORK.WIFI.ENABLE","YES");
        xmlconfig->setValue("NETWORK.WIFI.AP_MODE","NO");
        res=net_wifi_Reset();
        if(res) return res;
    }

    cmd=cmd+"CONNECT "+ssid+" "+pass;
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Connect()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        if(system(cmd.c_str())==-1)
        {
            //syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Connect()  system() error!");
        }
        //cout<<cmd<<endl;
        exit(0);
    }
*/
    return RET_OK;
}

int command::net_wifi_Scan()
{
    string cmd;
    //string value;
    //system("/usr/bin/geo/scripts/wifi_scanning.sh &");  //会先杀死hostapd，再搜索

    cmd.assign("/geo/app/scripts/wifi_scanning.sh ");
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_wifi_Scan()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        system(cmd.c_str());
        exit(0);
    }

#if 0
    char ifname[6] = "wlan0";
    char buffer[1024];		/* Results */
    int skfd;			/* generic raw socket desc.	*/
    struct ifreq ifr;

    system("kill `ps | grep \"B\" | grep \"/etc/hostapd.conf\" | awk '{print $1}'`");
    system("kill `ps | grep WIFI | grep udhcpd | awk '{print $1}'`");

    skfd=iw_sockets_open();

    strcpy(ifr.ifr_name, ifname);   //设置获取设备的名称，必须!!
    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"wlan get info error!");
        return -1;
    }
    ifr.ifr_flags |= IFF_UP;
    if(ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0)            //启动wifi
    {
        syslog(LOG_LOCAL7|LOG_ERR,"wlan set up error!");
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    get_scanning_ssid(skfd, ifname, 5000000, buffer, sizeof(buffer));

    //printf("%s\n", buffer);
    //printf("len: %d\n", strlen(buffer));
    iw_sockets_close(skfd);

    xmlconfig->setValue("NETWORK.WIFI.LIST",buffer);
    xmlconfig->saveConfig();

    net_wifi_Reset();
#endif
    return RET_OK;
}

int command::net_wifi_IP()
{
	xmlconfig->setValue("NETWORK.WIFI.IP",param);

	if(is_validIPv4Address(param.c_str())>0)
	{
		net_set_ddns();
	}

	return RET_OK;
}

int command::net_gprs()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return net_gprs_Reset();
    case CMD_STATUS:
        return net_gprs_Status();
	case CMD_IP:
		return net_gprs_IP();
    default:
        return -1;
    }
    return -1;
}

int command::net_gprs_IP()
{
	xmlconfig->setValue("NETWORK.GPRS.IP",param);

	if(is_validIPv4Address(param.c_str())>0)
	{
		net_set_ddns();
	}

	return RET_OK;
}

int command::net_gprs_Status()
{
    if(param.length()==0)
        return RET_ERR_PARAM;
    xmlconfig->setValue("NETWORK.GPRS.STATUS",param);

    static int gprs_reboot=0;
    static int gprs_status=0;
    syslog(LOG_LOCAL7|LOG_INFO,"gprs_reboot:%d gprs_status:%d || %s",gprs_reboot,gprs_status,param.c_str());
/**
430 :模块在初始化
431 :网络已准备好
435 :连上服务器
440 :模块关闭
*/
    if(atoi(param.c_str())==440&&gprs_status==0)
    {
        gprs_status=1;
        if(gprs_reboot++>10)
        {
#ifdef CN_VERSION
            dev_Reset();
#endif
        }

        if(base_started)ntrip_Reset();
        dev_Remote_Reset();
    }
    else if(atoi(param.c_str())==435)
    {
        gprs_reboot=0;
        gprs_status=0;
    }
    else
    {
        gprs_status=0;
    }

    return RET_OK;
}

int command::net_gprs_Reset()
{
    string value,apn,user,pass;
    string cmd;
    apn.clear();
    cmd.assign("/usr/bin/geo/scripts/gprs.sh ");
    xmlconfig->getValue("NETWORK.GPRS.ENABLE",&value);
    transform(value.begin(), value.end(), value.begin(), ::toupper);
    if(value.compare("YES") == 0)
    {
        if (xmlconfig->getValue("NETWORK.GPRS.APN",&apn)!= RET_OK|| apn.length()==0)
        {
            //cout<<"apn err"+apn<<endl;
            apn="\"\"";
        }

        if (xmlconfig->getValue("NETWORK.GPRS.USER",&user)!= RET_OK || user.length()==0)
            user="\"\"";
        if (xmlconfig->getValue("NETWORK.GPRS.PASS",&pass)!= RET_OK || pass.length()==0)
            pass="\"\"";
        cmd=cmd+"ENABLE "+apn+" "+user+" "+pass;
        //cout<<"apn "+apn<<endl;
    }
    else
    {
        cmd=cmd+"DISABLE";
    }
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"net_gprs_Reset()  fork() error!");
        return RET_EXEC_FAILED;
    }
    if (pid == 0)
    {
        system(cmd.c_str());
        exit(0);
    }
    return RET_OK;
}


int command::Radio()
{
    enPathField enpath =(enPathField)node[1];
    //printf("disk %d,%d \n", CMD_MOUNTMSD,enpath) ;
    switch(enpath)
    {
    case CMD_CHANNEL:
        return radio_channel();
    case CMD_POWER:
        return radio_power();
    case CMD_MODE:
        return radio_mode();
    case CMD_FREQUENCY:
        return radio_frequency();
    case CMD_RESET:
        return radio_Reset();
    case CMD_SELF_CHECK:
        return radio_selfcheck();
    default:
        return -1;
    }
    return -1;
}

int command::radio_change_channel(int channel)
{
    int n;
    char *start,*end;
    string cmd,freq;

    xmlconfig->getValue("RADIO.FREQUENCY",&freq);
    if(freq.length()==0) freq="438.125|440.125|441.125|442.125|443.125|444.125|446.125|447.125";

    //if(channel>0) channel-=1;
    start=(char *)freq.c_str();
    for(n=0; n<channel; n++)
    {
        if((end=strchr(start,'|'))>0)
        {
            start=end+1;
        }
        else
        {
            break;
        }
    }
    if((end=strchr(start,'|'))>0)
    {
        *end=0;
    }
    freq.assign(start);
    cmd="SET,RADIO.FREQ,"+freq+"\r\n";
    cout<<cmd<<endl;

    string ret;
    ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
	//Radio_command(
                (unsigned char *)cmd.c_str(),cmd.length(),&ret,6);

    if(ret.find("OK")!=std::string::npos)
        return RET_OK;
    else
    {
        system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
        sleep(1);
        system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
        ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
		//Radio_command(
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,8);
        if(ret.find("OK")!=std::string::npos)
            return RET_OK;
    }

    return RET_EXEC_FAILED;
}

int command::radio_channel()
{
    int n,channel;
    char *start,*end;
    string cmd,freq;
    xmlconfig->setValue("RADIO.CHANNEL",param);
    xmlconfig->saveConfig();
	syslog(LOG_LOCAL7|LOG_INFO,"%s > %s",__FUNCTION__,param.c_str());
    channel=atoi(param.c_str()); //从0开始
    xmlconfig->getValue("RADIO.FREQUENCY",&freq);
    if(freq.length()==0) freq="438.125|440.125|441.125|442.125|443.125|444.125|446.125|447.125";

    if(channel>0) channel-=1;
    start=(char *)freq.c_str();
    for(n=0; n<channel; n++)
    {
        if((end=strchr(start,'|'))>0)
        {
            start=end+1;
        }
        else
        {
            break;
        }
    }
    if((end=strchr(start,'|'))>0)
    {
        *end=0;
    }
    freq.assign(start);
    cmd="SET,RADIO.FREQ,"+freq+"\r\n";
    cout<<cmd<<endl;

    string ret;
    ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
	//Radio_command(
                (unsigned char *)cmd.c_str(),cmd.length(),&ret,6);

    if(ret.find("OK")!=std::string::npos)
        return RET_OK;
    else
    {
        system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
        sleep(1);
        system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
        ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
		//Radio_command(
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,8);
        if(ret.find("OK")!=std::string::npos)
            return RET_OK;
        syslog(LOG_LOCAL7|LOG_INFO,"%s > %s error",__FUNCTION__,param.c_str());
    }
    return RET_EXEC_FAILED;
}

int command::radio_power()
{
    string cmd;
    transform(param.begin(), param.end(), param.begin(), ::toupper);

    cmd="SET,RADIO.POWER,"+param+"\r\n";

    string ret;
    ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
	//Radio_command(
                (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);

    if(ret.find("OK")!=std::string::npos)
    {
        syslog(LOG_LOCAL7|LOG_INFO,"%s > %s OK",__FUNCTION__,param.c_str());
        xmlconfig->setValue("RADIO.POWER",param);
        xmlconfig->saveConfig();
        return RET_OK;
    }
    else
    {
        system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
        sleep(1);
        system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
        ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
		//Radio_command(
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);
        if(ret.find("OK")!=std::string::npos)
            return RET_OK;
        syslog(LOG_LOCAL7|LOG_INFO,"%s > %s error",__FUNCTION__,param.c_str());
    }
    return RET_EXEC_FAILED;
}

int command::radio_frequency()
{
    /* string uhf,cmd;
     float freq,freq_low,freq_up;
     xmlconfig->getValue("RADIO.INFO.MODEL",&uhf);
     freq_low=410;
     freq_up=470;
     if(uhf.find("XDL")!=std::string::npos)
     {
        freq_low=403;
        freq_up=473;
     }*/

    /*
         if(!SysParam.uhf_enable ||SysParam.cur_datalink!=MODE_LINK_UHF) ;
         else{
             string cmd;
             string area;
             xmlconfig->getValue("RADIO.AREA",&area);
             cmd="SET,RADIO.AREA,"+area+"\r\n";

            string ret;
            ipc_command(IPC_RADIO_CMD_WRITE,IPC_RADIO_CMD_READ,
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,2);
         }
    */

    xmlconfig->setValue("RADIO.FREQUENCY",param);
    return RET_OK;
}

int command::radio_mode()
{
    string uhf,cmd,mode;
    transform(param.begin(), param.end(), param.begin(), ::toupper);

    string model;
    string bw,bt;
    xmlconfig->getValue("RADIO.CHANNEL_SPACING",&bw);
    xmlconfig->getValue("RADIO.INFO.MODEL",&model);

    if(model.find("XDL")!=std::string::npos)
    {
        if(param=="0") mode="SATEL";
        else if(param=="2") mode="TRANSEOT";
        else if(param=="9") mode="TRIMMK3";
        else if(param=="8") mode="TRIMMK2";
        else if(param=="4") mode="SOUTH_9600";
        else if(param=="10") mode="SOUTH_19200";
        else  if(param=="3") mode="TRIMTALK";
        else return RET_ERR_PARAM;
    }
    else
    {
        if(param=="2")
        {
            /*if(bw=="12.5") mode="TRANSEOT_4800";
            else*/ mode="TRANSEOT";
        }
        else  if(param=="3")
        {
            /*if(bw=="12.5") mode="TRIMTALK_4800";
            else*/ mode="TRIMTALK";
            xmlconfig->setValue("RADIO.CHANNEL_SPACING","25");
            xmlconfig->setValue("RADIO.FEC","OFF");
        }
        else  if(param=="0")
        {
            if(bw=="25") mode="SATEL_19200";
            else
            {
                xmlconfig->setValue("RADIO.CHANNEL_SPACING","12.5");
                mode="SATEL_9600";
            }

        }
        else if(param=="9") mode="TRIMMK3_19200";
        else if(param=="4") mode="SOUTH_9600";
        else if(param=="10") mode="SOUTH_19200";
        else  if(param=="11")
        {
             mode="TRIMTALK_4800";
             xmlconfig->setValue("RADIO.CHANNEL_SPACING","12.5");
             xmlconfig->setValue("RADIO.FEC","OFF");
        }
        else if(param=="12")
        {
            mode="TRANSEOT_4800";
            xmlconfig->setValue("RADIO.CHANNEL_SPACING","12.5");
            //xmlconfig->setValue("RADIO.FEC","ON");
        }
        else  if(param=="13")
        {
            /*if(bw=="12.5") mode="TRIMTALK_4800";
            else*/ mode="GEOTALK";
            xmlconfig->setValue("RADIO.CHANNEL_SPACING","25");
            //xmlconfig->setValue("RADIO.FEC","ON");
        }
        else  if(param=="14")
        {
            /*if(bw=="12.5") mode="TRIMTALK_4800";
            else*/ mode="GEOMK3_19200";
            xmlconfig->setValue("RADIO.CHANNEL_SPACING","25");
            //xmlconfig->setValue("RADIO.FEC","ON");
        }
        else return RET_ERR_PARAM;
    }
    cmd="SET,RADIO.MODE,"+mode+"\r\n";

    string ret;
    ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
	//Radio_command(
                (unsigned char *)cmd.c_str(),cmd.length(),&ret,6);

    if(ret.find("OK")!=std::string::npos)
    {
        syslog(LOG_LOCAL7|LOG_INFO,"%s > %s OK",__FUNCTION__,param.c_str());
        xmlconfig->setValue("RADIO.MODE",param);
        xmlconfig->saveConfig();

        //set CHANNEL_SPACING

        if(model.find("U1006")!=std::string::npos)
        {
#if 0
            cmd="SET,RADIO.BW,"+bw+"\r\n";
            ipc_command(IPC_RADIO_CMD_WRITE,IPC_RADIO_CMD_READ,
			//Radio_command(
                        (unsigned char *)cmd.c_str(),cmd.length(),&ret,3);

            bt=".5";
            if(mode.find("19200")!=std::string::npos)
            {
                bw="25";
                bt=".3";
            }
            else
            {
                if(bw.find("12.5")!=std::string::npos) bt=".3";
                else bt=".5";
            }
            cmd="SET,RADIO.BT,"+bt+"\r\n";
            ipc_command(IPC_RADIO_CMD_WRITE,IPC_RADIO_CMD_READ,
			//Radio_command(
                        (unsigned char *)cmd.c_str(),cmd.length(),&ret,3);
#endif
        }
        else if(model.find("XDL")!=std::string::npos)
        {

            cmd="SET,RADIO.BW,"+bw+"\r\n";
            ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
			//Radio_command(
                        (unsigned char *)cmd.c_str(),cmd.length(),&ret,3);

            string opt,value;
            opt.empty();

            if(mode.find("SATEL")!=std::string::npos||mode.find("SOUTH")!=std::string::npos)
            {
                /*
                cmd="SET,RADIO.MODE,"+mode+"\r\n";
                ipc_command(IPC_RADIO_CMD_WRITE,IPC_RADIO_CMD_READ,
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,6);*/
                opt+="PROTOCOL_"+mode+"|";
            }

            xmlconfig->getValue("RADIO.AREA",&value);
            if(value.length()==0) value="9";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+="COUNTRY_"+value+"|";

            xmlconfig->getValue("RADIO.MODULATION",&value);//4FSK GMSK
            if(value.length()==0) value="GMSK";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+=value+"|";

            xmlconfig->getValue("RADIO.LINKSPEED",&value);//4800 9600
            if(value.length()==0) value="9600";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+="LINKSPEED_"+value+"|";

            xmlconfig->getValue("RADIO.CSMA",&value);//ON OFF
            if(value.length()==0) value="ON";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+="CSMA_"+value+"|";

            xmlconfig->getValue("RADIO.FEC",&value);//ON OFF
            if(value.length()==0) value="OFF";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+="FEC_"+value+"|";

            xmlconfig->getValue("RADIO.SCRAMBLER",&value);//ON OFF
            if(value.length()==0) value="ON";
            transform(value.begin(), value.end(), value.begin(), ::toupper);
            opt+="SCRAMBLER_"+value+"|";

            cmd="SET,RADIO.OPT,"+opt+"\r\n";
            sleep(1);
            ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
			//Radio_command(
                        (unsigned char *)cmd.c_str(),cmd.length(),&ret,3);
        }
        else if(model.find("TRM10")!=std::string::npos)
        {
            string value;
            if(mode.find("SATEL")!=std::string::npos)
            {
                xmlconfig->getValue("RADIO.FEC",&value);//ON OFF
                if(value.length()==0) value="OFF";
                transform(value.begin(), value.end(), value.begin(), ::toupper);

                cmd="SET,RADIO.FEC,"+value+"\r\n";
                ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
				//Radio_command(
                            (unsigned char *)cmd.c_str(),cmd.length(),&ret,3);
            }
        }
        return RET_OK;
    }
    else
    {
        system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
        sleep(1);
        system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
        ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
		//Radio_command(
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);
        if(ret.find("OK")!=std::string::npos)
            return RET_OK;
        syslog(LOG_LOCAL7|LOG_INFO,"%s > %s error",__FUNCTION__,param.c_str());
    }
    return RET_EXEC_FAILED;
}

int command::radio_selfcheck()
{
    string cmd;
    cmd="SET,RADIO.SELF_CHECK\r\n";

    string ret;
    size_t start,end;

    ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
	//Radio_command(
                (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);

    if(ret.find("OK")==std::string::npos)
    {
        clear_msg(PROC_INTERFACE);
        sleep(5);
        clear_msg(PROC_INTERFACE);
        ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
		//Radio_command(
                    (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);
        if(ret.find("OK")==std::string::npos)
        {
            clear_msg(PROC_INTERFACE);
            sleep(10);
            clear_msg(PROC_INTERFACE);
            ipc_command(IPC_RADIO_CMD,IPC_RADIO_CMD_RESPONSE,
			//Radio_command(
                        (unsigned char *)cmd.c_str(),cmd.length(),&ret,5);
            if(ret.find("OK")==std::string::npos)
            {
                syslog(LOG_LOCAL7|LOG_ERR,"UHF:%s\n",ret.c_str());
            }
        }
    }

    //syslog(LOG_LOCAL7|LOG_INFO,"UHF: %s\n",ret.c_str());
    if(ret.find("OK")!=std::string::npos)
    {
        if((start=ret.find("SN:"))!=std::string::npos)
        {
            start+=3;
            if((end=ret.find("|",start))!=std::string::npos)
            {
                xmlconfig->setValue("RADIO.INFO.SERIAL",ret.substr(start,end-start));
            }
        }
        if((start=ret.find("FW:"))!=std::string::npos)
        {
            start+=3;
            if((end=ret.find("|",start))!=std::string::npos)
            {
                xmlconfig->setValue("RADIO.INFO.FIRMWARE_VER",ret.substr(start,end-start));
            }
        }
        return RET_OK;
    }
    else
        return RET_EXEC_FAILED;
}

int command::radio_Reset()
{
    char* exec_argv[8] = {0};
    unsigned int n;
    string model;
    xmlconfig->getValue("RADIO.INFO.MODEL",&model);
    if(model.length()==0) model="U1006";
    if(SysParam.direct_uhf==0) model="-";
    if(model.find("TRM10")!=std::string::npos) model="U1006";
    if(model.find("U1007")!=std::string::npos) model="DU1007D";
    //start radio

    if(SysParam.radio)
    {
        string radio_en;
        xmlconfig->getValue("PORTS.RADIO.ENABLE",&radio_en);
        transform(radio_en.begin(), radio_en.end(), radio_en.begin(), ::toupper);

        if(radio_en.compare("YES") != 0 )
        {
            stop_process(process[PROCESS_RADIO]);

            system((char *)"rmmod uhf-enable  1> /dev/null  2>&1") ;
            system((char *)"rmmod uhf_config  1> /dev/null  2>&1") ;
            SysParam.uhf_enable=0;
            return RET_OK;
        }
    }else
    {
        syslog(LOG_LOCAL7|LOG_INFO,"No UHF Model .");
        return RET_OK;
    }
    SysParam.uhf_enable=1;

    //default_cpld_mode();
	ctl_setDataLink(MODE_LINK_1);
    system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1") ;
    if(SysParam.direct_uhf)
    {
        system((char *)"insmod /lib/modules/uhf-config.ko  1> /dev/null  2>&1");
        //system((char *)"rmmod uhf-config  1> /dev/null  2>&1");
    }

    exec_argv[0] = BuildInfoFromString(RADIO_BIN_PATH);
	if(SysParam.hardver>=4)
	{
		exec_argv[1] = BuildInfoFromString(RADIO_SERIAL_V4);
	}else
	{
		exec_argv[1] = BuildInfoFromString(RADIO_SERIAL);
	}
	exec_argv[2] = BuildInfoFromString(IPC_RADIO_WRITE_INFO);
	exec_argv[3] = BuildInfoFromString(IPC_RADIO_READ_INFO);
    exec_argv[4] = BuildIPCInfo(IPC_RADIO_CMD,IPC_CMD_SIZE8K);
	exec_argv[5] = BuildIPCInfo(IPC_RADIO_CMD_RESPONSE,IPC_CMD_SIZE8K);
    exec_argv[6] = BuildInfoFromString(model.c_str());
    start_process(7,exec_argv,process[PROCESS_RADIO]);

    return RET_OK;
}

int command::Ports()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_RESET:
        return port_Reset();
    case CMD_BLUETOOTH:
        return port_Bluetooth();
    case CMD_SOCKET0_RAW:
        return port_socket(0);
    case CMD_SOCKET1_RAW:
        return port_socket(1);
    case CMD_SOCKET2_RAW:
        return port_socket(2);
    default:
        return -1;
    }
    return -1;
}


int command::port_socket(int num)
{
    ENDPOINT_OPT endpoint;
    string interval, raw_eph;
    string func;

 //syslog(LOG_LOCAL7|LOG_INFO,"SOCKET0: PROCESS_TCP0_O %d\n", endpoint.id);

    switch(num)
    {
    case 0:
        if(find_endpoint(&endpoint, PROCESS_TCP_O))
        {
            func="";
            xmlconfig->getValue("PORTS.SOCKET.FUNCTION",&func);
            transform(func.begin(), func.end(), func.begin(), ::toupper);
            if(func.compare("RAW") == 0 || func.compare("BINEX") == 0)
            {
                xmlconfig->getValue("PORTS.SOCKET.RAW",&interval);
                xmlconfig->getValue("PORTS.SOCKET.RAW_EPH",&raw_eph);
                //oem->issueRAW(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                oem->issueRAWEPH(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                syslog(LOG_LOCAL7|LOG_INFO,"SOCKET0: PROCESS_TCP0_O %d\n", endpoint.id);
            }
        }
        break;
    case 1:
        if(find_endpoint(&endpoint, PROCESS_TCP1_O))
        {
            func="";
            xmlconfig->getValue("PORTS.SOCKET1.FUNCTION",&func);
            transform(func.begin(), func.end(), func.begin(), ::toupper);
            if(func.compare("RAW") == 0 || func.compare("BINEX") == 0)
            {
                xmlconfig->getValue("PORTS.SOCKET1.RAW",&interval);
                xmlconfig->getValue("PORTS.SOCKET1.RAW_EPH",&raw_eph);
                //oem->issueRAW(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                oem->issueRAWEPH(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                syslog(LOG_LOCAL7|LOG_INFO,"SOCKET1 PROCESS_TCP1_O %d\n", endpoint.id);
            }
        }
        break;
    case 2:
        if(find_endpoint(&endpoint, PROCESS_TCP2_O))
        {
            func="";
            xmlconfig->getValue("PORTS.SOCKET2.FUNCTION",&func);
            transform(func.begin(), func.end(), func.begin(), ::toupper);
            if(func.compare("RAW") == 0 || func.compare("BINEX") == 0)
            {
                xmlconfig->getValue("PORTS.SOCKET2.RAW",&interval);
                xmlconfig->getValue("PORTS.SOCKET2.RAW_EPH",&raw_eph);
                //oem->issueRAW(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                oem->issueRAWEPH(endpoint.id,atoi(interval.c_str()),atoi(raw_eph.c_str()));
                syslog(LOG_LOCAL7|LOG_INFO,"SOCKET2 PROCESS_TCP2_O  %d\n", endpoint.id);
            }
        }
        break;
    default:
        break;
    }
    return RET_OK;
}


int command::port_Bluetooth()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_ENABLE:
        return port_BluetoothEnable();
    case CMD_CONNECTED:
        return port_BluetoothConnected();
    case CMD_RESET:
        return port_BluetoothReset();
    default:
        return -1;
    }
    return -1;
}

int command::port_Reset()
{
    char* exec_argv[8] = {0};
    unsigned int n;
    string func,option;
    int setup_success=0;
    ENDPOINT_OPT endpoint;
	char * szPortXMLName;
	char szTemp[40];
#ifdef CheckRegi
    //if(SysParam.reg_state!=REG_OK)
    //    return RET_INVALID_OPERATION;
#endif

    option="_";
    string gpsmodel;
    xmlconfig->getValue("DEVICE.INFO.GPSBOARD",&gpsmodel);
    option+="_OEM-"+gpsmodel;

#if 1//bluetooth
	szPortXMLName="BLUETOOTH";
    setup_success=0;
    if(process[PROCESS_BT_O])
    {
            if(release_endpoint(&endpoint,PROCESS_BT_O))
                oem->unlogall(endpoint.id);
    }
    if(process[PROCESS_BT_BINEX]>0)stop_process(process[PROCESS_BT_BINEX]);
    string bt_en;
	GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&bt_en);
    if(bt_en.compare("YES") != 0 )
    {
        setup_success=1;
    }
    else
    {
		func="";
		GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
		//szIPCReadInfo IPC_BT_READ_INFO:where cmd/diff data read from
		//szIPCWriteInfo IPC_BT_WRITE_INFO:where cmd write response/gnss data or gnss data
		setup_success=StartPortWithConfigedFunction(szPortXMLName,func,&endpoint,
			PROCESS_BT_O,IPC_BT_READ_INFO,IPC_BT_WRITE_INFO,
			PROCESS_BT_BINEX,IPC_BT_WRITE_BINEX_INFO,
			CMD_BT_ID,option,0);
    }
    if(setup_success==0)
        xmlconfig->setValue("PORTS.BLUETOOTH.STATUS","error");
    else
        xmlconfig->setValue("PORTS.BLUETOOTH.STATUS","idle");
    clear_msg(PROC_INTERFACE);
#endif

    //RADIO
    if(SysParam.radio)
    {
        string radio_en;
		szPortXMLName="RADIO";
		GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&radio_en);
        syslog(LOG_LOCAL7|LOG_INFO,"Have radio model  > enable : %s",radio_en.c_str());

        if(process[PROCESS_RADIO_O]>0)
        {
            if(find_endpoint(&endpoint,PROCESS_RADIO_O))
                oem->unlogall(endpoint.id);
            if(release_endpoint(&endpoint,PROCESS_RADIO_O));
                //oem->unlogall(endpoint.id);
        }
        if(process[PROCESS_RADIO_BINEX]>0)stop_process(process[PROCESS_RADIO_BINEX]);
        if(radio_en.compare("YES") != 0 )
        {

        }
        else
        {
			func="";
			GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
			//szIPCReadInfo IPC_RADIO_WRITE_INFO:where cmd/diff data read from
			//szIPCWriteInfo IPC_RADIO_READ_INFO:where cmd write response/gnss data or gnss data
			setup_success=StartPortWithConfigedFunction(szPortXMLName,func,&endpoint,
				PROCESS_RADIO_O,IPC_RADIO_READ_INFO,IPC_RADIO_WRITE_INFO,
				PROCESS_RADIO_BINEX,IPC_RADIO_WRITE_BINEX_INFO,
				CMD_BT_ID,option,0);

        }
    }

#if 1
     //COM1     //****************************
	szPortXMLName="COM1";
    setup_success=0;
    string com_en;
	GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&com_en);
	if(process[PROCESS_COM_O]>0)
	{
		if(release_endpoint(&endpoint,PROCESS_COM_O)){
			oem->unlogall(endpoint.id);
		}
	}
    if(process[PROCESS_COM_BINEX]>0)stop_process(process[PROCESS_COM_BINEX]);
    if(ntrip_double)
    {
        //set_ntrip_connect(0);
		if(process[PROCESS_HUB]>0)stop_process(process[PROCESS_HUB]);
        ntrip_double=0;
    }
    if(com_en.compare("YES") != 0 )
    {
        if(process[PROCESS_COM])stop_process(process[PROCESS_COM]);
        setup_success=1;
        //default_cpld_mode();
		ctl_setDataLink(MODE_LINK_1);
    }
    else
    {
		//end point: data source
		func="";
		GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
		//szIPCReadInfo IPC_COM_READ_INFO:where cmd/diff data read from
		//szIPCWriteInfo IPC_COM_WRITE_INFO:where cmd write response/gnss data or gnss data
		setup_success=StartPortWithConfigedFunction(szPortXMLName,func,&endpoint,
			PROCESS_COM_O,IPC_COM_READ_INFO,IPC_COM_WRITE_INFO,
			PROCESS_COM_BINEX,IPC_COM_WRITE_BINEX_INFO,
			CMD_COM_ID,option,0);

        string baud;
		GetPortXMLValue("PORTS.%s.BAUDRATE",szPortXMLName,&baud);
        if(baud.length()==0) baud="115200";

        if(process[PROCESS_COM])
        {
            if(com1_baud!=atoi(baud.c_str()))
            {
                stop_process(process[PROCESS_COM]);
                com1_baud=atoi(baud.c_str());
            }
        }
		//start a process on ttyUSB(COM1),read IPC_COM_WRITE_INFO to ttyUSB, read ttyUSB to IPC_COM_READ_INFO
        if(process[PROCESS_COM]==0)
        {
            char *COM;

			if(SysParam.hardver>=4)
			{
				COM=COM_SERIAL;
			}
			else
			{
				if(rs485_flag==0 || ((SysParam.hardver==2||SysParam.hardver==3) && gpsboard==GPS_HEMISPHERE))
					COM=COM_SERIAL;
				else{
					if(gpsboard==GPS_HEMISPHERE)
						COM= "ttyUSB/cp210x1";
					else
						COM= "ttyUSB/cp210x0";
				}
			}

			StartProcess_Serial2(COM,baud.c_str(),IPC_COM_WRITE_INFO,IPC_COM_READ_INFO,process[PROCESS_COM]);
			printf("StartProcess_Serial2 com1 %d\n",process[PROCESS_COM]);
        }



		if (setup_success==-1)
		{
			//did not processed by StartPortWithConfigedFunction
			if(func.compare("NTRIP_OUT") == 0 )
			{
				//if(process[PROCESS_HUB]==0)
				set_ntrip_connect(0);
				setup_success=1;
				ntrip_double=1;
			}
		}
		//CPLD change
		if(func.compare("GPS") == 0 )
		{
			ctl_setDataLink(MODE_LINK_GPS);
/*
#ifdef NSC200
			system("rmmod cpld-mode1 1> /dev/null  2>&1");
			system("rmmod cpld-mode4 1> /dev/null  2>&1");
			system("insmod /lib/modules/cpld-mode2.ko 1> /dev/null  2>&1");
			if((rs485_flag==1)&&(gpsboard!=GPS_HEMISPHERE))
				system("insmod /lib/modules/cpld-mode3.ko 1> /dev/null  2>&1");
			else
				system("rmmod cpld-mode3 1> /dev/null  2>&1");
			if(gpsboard==GPS_HEMISPHERE)
			{
				direct_link=1;
			}
#else
			system("rmmod gpio158 1> /dev/null  2>&1");
			system("rmmod gpio160 1> /dev/null  2>&1");
			system("insmod /lib/modules/gpio159.ko 1> /dev/null  2>&1");
#endif
*/
		}
		else if(func.compare("UHF") == 0 )
		{
			ctl_setDataLink(MODE_LINK_UHF);
			/*
			system("rmmod cpld-mode1 1> /dev/null  2>&1");
			system("rmmod cpld-mode2 1> /dev/null  2>&1");
			system("rmmod cpld-mode3 1> /dev/null  2>&1");
			system("insmod /lib/modules/cpld-mode4.ko 1> /dev/null  2>&1");
			system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1");
			if(SysParam.direct_uhf)
			{
				system((char *)"insmod /lib/modules/uhf-config.ko  1> /dev/null  2>&1");
				system((char *)"rmmod uhf-config  1> /dev/null  2>&1");
			}*/
		}
		else
		{
			//default_cpld_mode();
			ctl_setDataLink(MODE_LINK_1);
		}
	}
	if(setup_success==0)
        xmlconfig->setValue("PORTS.COM1.STATUS","error");
    else
        xmlconfig->setValue("PORTS.COM1.STATUS","idle");
    clear_msg(PROC_INTERFACE);
#endif

     //COM2     //****************************
	szPortXMLName="COM2";
    if((rs485_flag==1)&&(SysParam.hardver>=4||gpsboard!=GPS_HEMISPHERE)){
        setup_success=0;
        string com2_en;
		GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&com2_en);
        if(process[PROCESS_COM2_O]>0)
        {
            if(release_endpoint(&endpoint,PROCESS_COM2_O)){
                oem->unlogall(endpoint.id);
            }
        }
        if(process[PROCESS_COM2_BINEX]>0)stop_process(process[PROCESS_COM2_BINEX]);
        if(com2_en.compare("YES") != 0 )
        {
            setup_success=1;   //default_cpld_mode();
            if(process[PROCESS_COM2])stop_process(process[PROCESS_COM2]);
        }
        else
        {
            string baud;
			GetPortXMLValue("PORTS.%s.BAUDRATE",szPortXMLName,&baud);
            if(baud.length()==0) baud="115200";

            if(process[PROCESS_COM2])
            {
                if(com2_baud!=atoi(baud.c_str()))
                {
                    stop_process(process[PROCESS_COM2]);
                    com2_baud=atoi(baud.c_str());
                }
            }

			char *COM;
			if(SysParam.hardver>=4)
			{
				COM="ttyO5";
			}
			else
			{
				COM="ttyO4";
			}

			func="";
			GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
			if(func.find("MET") !=std::string::npos )
			{
				setup_success=1;
				StartProcess_Met(COM,baud.c_str(),IPC_MET_WRITE_INFO,process[PROCESS_COM2]);
			}
			else
			{
				//start a process on ttyO4,read IPC_COM2_WRITE_INFO to tty04, read tty04 to IPC_COM2_READ_INFO
				if(process[PROCESS_COM2]==0)
				{
					StartProcess_Serial2(COM,baud.c_str(),IPC_COM2_WRITE_INFO,IPC_COM2_READ_INFO,process[PROCESS_COM2]);
				}

				//szIPCReadInfo IPC_COM2_READ_INFO:where cmd/diff data read from
				//szIPCWriteInfo IPC_COM2_WRITE_INFO:where cmd write response/gnss data or gnss data
				setup_success=StartPortWithConfigedFunction(szPortXMLName,func,&endpoint,
					PROCESS_COM2_O,IPC_COM2_READ_INFO,IPC_COM2_WRITE_INFO,
					PROCESS_COM2_BINEX,IPC_COM2_WRITE_BINEX_INFO,
					CMD_COM2_ID,option,0);
			}

            if (setup_success==-1)
            {
				if(func.compare("NTRIP_OUT") == 0 )
				{
					setup_success=1;
				}
            }
        }
        if(setup_success==0)
            xmlconfig->setValue("PORTS.COM2.STATUS","error");
        else
            xmlconfig->setValue("PORTS.COM2.STATUS","idle");
        clear_msg(PROC_INTERFACE);
    }else{
        xmlconfig->setValue("PORTS.COM2.ENABLE","NO");
    }

	//COM3     //****************************
	szPortXMLName="COM3";
    setup_success=0;
    string com3_en;
	GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&com3_en);
	if(process[PROCESS_COM3_O]>0)
	{
		if(release_endpoint(&endpoint,PROCESS_COM3_O)){
			oem->unlogall(endpoint.id);
		}
	}
    if(process[PROCESS_COM3_BINEX]>0)stop_process(process[PROCESS_COM3_BINEX]);

	system("rm /boot/debug");
    if(com3_en.compare("YES") != 0 )
    {
        if(process[PROCESS_COM3])stop_process(process[PROCESS_COM3]);
        setup_success=1;
    }
    else
    {
		string baud;
		GetPortXMLValue("PORTS.%s.BAUDRATE",szPortXMLName,&baud);
		if(baud.length()==0) baud="115200";

		if(process[PROCESS_COM3])
		{
			if(com3_baud!=atoi(baud.c_str()))
			{
				stop_process(process[PROCESS_COM3]);
				com3_baud=atoi(baud.c_str());
			}
		}
		//start a process on ttyO0,read IPC_COM3_WRITE_INFO to ttyO0, read ttyO0 to IPC_COM3_READ_INFO
		if(process[PROCESS_COM3]==0)
		{
			StartProcess_Serial2(COM3_SERIAL,baud.c_str(),IPC_COM3_WRITE_INFO,IPC_COM3_READ_INFO,process[PROCESS_COM3]);
		}

		//end point: data source
		func="";
		GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
		//szIPCReadInfo IPC_COM3_READ_INFO:where cmd/diff data read from
		//szIPCWriteInfo IPC_COM3_WRITE_INFO:where cmd write response/gnss data or gnss data
		setup_success=StartPortWithConfigedFunction(szPortXMLName,func,&endpoint,
			PROCESS_COM3_O,IPC_COM3_READ_INFO,IPC_COM3_WRITE_INFO,
			PROCESS_COM3_BINEX,IPC_COM3_WRITE_BINEX_INFO,
			CMD_COM3_ID,option,0);
	}
	if(setup_success==0)
		xmlconfig->setValue("PORTS.COM3.STATUS","error");
	else
		xmlconfig->setValue("PORTS.COM3.STATUS","idle");
	clear_msg(PROC_INTERFACE);

    //Ntrip Client //**********************************
    string  ntripclient_en;
    string  ntrip_en;
    setup_success=0;
    if(process[PROCESS_NTRIP_CLIENT_O]>0)
    {
        if(release_endpoint(&endpoint,PROCESS_NTRIP_CLIENT_O))
            oem->unlogall(endpoint.id);
    }
    xmlconfig->getValue("PORTS.NTRIP_CLIENT.ENABLE",&ntrip_en);
    transform(ntrip_en.begin(), ntrip_en.end(), ntrip_en.begin(), ::toupper);
    if(ntrip_en.compare("YES") != 0 )
    {
        setup_success=1;
        if(process[PROCESS_NTRIP_CLIENT]) stop_process(process[PROCESS_NTRIP_CLIENT]);
    }
    else
    {
        string addr,user,pass,userpass,mountpoint,ntripversion;
		//read nmea from gnss board to IPC_NTRIP_CLIENT_WRITE_INFO,
		//read cors dif data from IPC_NTRIP_CLIENT_READ_INFO to gnss board
        //if(creat_endpoint(&endpoint,PROCESS_NTRIP_CLIENT_O,0,IPC_NTRIP_CLIENT_WRITE,IPC_NTRIP_CLIENT_READ))
		if(creat_endpoint(&endpoint,PROCESS_NTRIP_CLIENT_O,0,IPC_NTRIP_CLIENT_READ_INFO,IPC_NTRIP_CLIENT_WRITE_INFO))
        {
			setup_success=1;

			string gga;
			char tmpStr[64];
			int ggaint;

			if (xmlconfig->getValue("PORTS.NTRIP_CLIENT.UPLOADGGA",&gga)!= RET_OK||gga.length()==0)
				gga="10000";
			ggaint=atoi(gga.c_str());

			if(ggaint>0)
			{
				sprintf(tmpStr,"GGA:%d",ggaint);
				if(oem->lognmea(endpoint.id,(char *)tmpStr));
			}
            oem->rtkmode(endpoint.id);

            xmlconfig->getValue("PORTS.NTRIP_CLIENT.ADDR",&addr);
            if(addr.length()==0) addr="192.168.1.9:6060";

            xmlconfig->getValue("PORTS.NTRIP_CLIENT.USER",&user);
            if(user.length()==0) user="test";
            xmlconfig->getValue("PORTS.NTRIP_CLIENT.PASS",&pass);
            if(pass.length()==0) pass="1234";
            userpass=user+"|"+pass;
            xmlconfig->getValue("PORTS.NTRIP_CLIENT.MOUNTPOINT",&mountpoint);
            if(mountpoint.length()==0) mountpoint="TEST";
            if (xmlconfig->getValue("PORTS.NTRIP_CLIENT.NTRIPVERSION",&ntripversion))
            if(ntripversion.length()==0) ntripversion="NTRIPV1";

            //read nmea from IPC_NTRIP_CLIENT_WRITE_INFO to cors server
			//read cors dif data from cors server to IPC_NTRIP_CLIENT_READ_INFO
            exec_argv[0] = BuildInfoFromString(NTRIP_CLIENT_BIN_PATH);
            exec_argv[1] = BuildInfoFromString(addr.c_str());
            exec_argv[2] = BuildInfoFromString(userpass.c_str());
            exec_argv[3] = BuildInfoFromString(mountpoint.c_str());
			exec_argv[4] = BuildInfoFromString(IPC_NTRIP_CLIENT_WRITE_INFO);
			exec_argv[5] = BuildInfoFromString(IPC_NTRIP_CLIENT_READ_INFO);
            exec_argv[6] = BuildInfoFromString(ntripversion.c_str());
            start_process(7,exec_argv,process[PROCESS_NTRIP_CLIENT]);
        }
    }
    if(setup_success==0)
        xmlconfig->setValue("PORTS.NTRIP_CLIENT.STATUS","error");
    else
        xmlconfig->setValue("PORTS.NTRIP_CLIENT.STATUS","idle");
    clear_msg(PROC_INTERFACE);

    //NTRIP CASTER //**********************************
    string  ntripcaster_en;
    setup_success=0;

    xmlconfig->getValue("PORTS.NTRIP_CASTER.ENABLE",&ntripcaster_en);
    transform(ntripcaster_en.begin(), ntripcaster_en.end(), ntripcaster_en.begin(), ::toupper);
    if(ntripcaster_en.compare("YES") != 0 )
    {
        setup_success=1;
        if(process[PROCESS_NTRIP_CASTER]) stop_process(process[PROCESS_NTRIP_CASTER]);
    }
    else
    {
        string addr_port;
        setup_success=1;

        xmlconfig->getValue("PORTS.NTRIP_CASTER.ADDR",&addr_port);
        if(addr_port.length()==0) addr_port="2012";

        exec_argv[0] = BuildInfoFromString(NTRIP_CASTER_BIN_PATH);
        exec_argv[1] = BuildInfoFromString(addr_port.c_str());
        start_process(2,exec_argv,process[PROCESS_NTRIP_CASTER]);
    }
    if(setup_success==0)
        xmlconfig->setValue("PORTS.NTRIP_CASTER.STATUS","error");
    else
        xmlconfig->setValue("PORTS.NTRIP_CASTER.STATUS","idle");
    clear_msg(PROC_INTERFACE);

    //SOCKET //**********************************
	szPortXMLName="SOCKET";
	//SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP_O,PROCESS_TCP,IPC_TCP_WRITE_INFO,IPC_TCP_READ_INFO,PROCESS_TCP_BINEX,IPC_TCP_WRITE_BINEX_INFO,CMD_TCP_ID,option);
	SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP_O,
		PROCESS_TCP,IPC_TCP_WRITE,IPC_TCP_READ_INFO,
		PROCESS_TCP_BINEX,IPC_TCP_WRITE_BINEX,CMD_TCP_ID,option);

	//SOCKET1 //**********************************
	szPortXMLName="SOCKET1";
	//SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP1_O,PROCESS_TCP1,IPC_TCP1_WRITE_INFO,IPC_TCP1_READ_INFO,PROCESS_TCP1_BINEX,IPC_TCP1_WRITE_BINEX_INFO,CMD_TCP1_ID,option);
	SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP1_O,
		PROCESS_TCP1,IPC_TCP1_WRITE,IPC_TCP1_READ_INFO,
		PROCESS_TCP1_BINEX,IPC_TCP1_WRITE_BINEX,CMD_TCP1_ID,option);

    //SOCKET2 //**********************************
	szPortXMLName="SOCKET2";
	//SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP2_O,PROCESS_TCP2,IPC_TCP2_WRITE_INFO,IPC_TCP2_READ_INFO,PROCESS_TCP2_BINEX,IPC_TCP2_WRITE_BINEX_INFO,CMD_TCP2_ID,option);
	SetupSocketPort(szPortXMLName,&endpoint,PROCESS_TCP2_O,
		PROCESS_TCP2,IPC_TCP2_WRITE,IPC_TCP2_READ_INFO,
		PROCESS_TCP2_BINEX,IPC_TCP2_WRITE_BINEX,CMD_TCP2_ID,option);
    return RET_OK;

}

int command::port_BluetoothEnable()
{
    if(param.empty())
    {
        return RET_ERR_PARAM;
    }

    string cmd;
    transform(param.begin(), param.end(), param.begin(), ::toupper);
    xmlconfig->setValue("PORTS.BLUETOOTH.ENABLE",param);


    return port_BluetoothReset();
}


int command::port_BluetoothConnected()
{
    if(param.empty())
    {
        return RET_ERR_PARAM;
    }

    transform(param.begin(), param.end(), param.begin(), ::toupper);
    xmlconfig->setValue("PORTS.BLUETOOTH.CONNECTED",param);

    if(param.compare("1") == 0 )//bt connect
    {
        if(process[PROCESS_BT]) stop_process(process[PROCESS_BT]);
		StartProcess_Serial2(BT_SERIAL,"115200",IPC_BT_WRITE_INFO,IPC_BT_READ_INFO,process[PROCESS_BT]);
    }
    else
    {
        if(process[PROCESS_BT]) stop_process(process[PROCESS_BT]);
    }

    return RET_OK;
}

int command::port_BluetoothReset()
{
    string value;
    string cmd;

    xmlconfig->getValue("PORTS.BLUETOOTH.ENABLE",&value);
    cmd.assign("/usr/bin/geo/scripts/bluetooth.sh ");
    transform(value.begin(), value.end(), value.begin(), ::toupper);
    if(value.compare("YES") == 0)
    {
        string name;
        if (xmlconfig->getValue("DEVICE.INFO.SERIAL",&name)!= RET_OK||name.length()==0)
            name="SC200";
        cmd=cmd+"ENABLE "+name;
        if(bluetooth_enable) return RET_OK;
        bluetooth_enable=1;

    }
    else
    {
        cmd=cmd+"DISABLE";
        if(!bluetooth_enable) return RET_OK;
        bluetooth_enable=0;
    }

    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"BluetoothReset()  fork() error!");
        return false;
    }
    if (pid == 0)
    {
        system(cmd.c_str());
        //cout<<cmd<<endl;
        exit(0);
    }
    return RET_OK;
}


bool command::init_dev()
{
    int res=RET_OK;

    net_wifi_Reset();
    net_gprs_Reset();
    net_wan_Reset();
    net_dns1_Reset();
    net_dnat_Reset();
    net_snat_Reset();
    res+=net_priority_Reset();
    res+=net_dns1_Reset();

    return (res==RET_OK);
}

bool command::default_setting()
{
    int res=RET_OK;
	string value;

	xmlconfig->getValue("SYSTEM.TIMEZONE",&value);
	if(value.length()>0 && (value.find("AUTO")==string::npos))
	{
		set_sysTimezone(atoi(value.c_str()));
	}

    res+=gps_default_setting();
    res+=port_BluetoothReset();
	if(SysParam.radio)res+=radio_Reset();
    res+=port_Reset();
	net_ftp_Reset();
	net_set_ddns();
    if(check_usbconnect())
    {
        usb_connected=1;
        syslog(LOG_LOCAL7|LOG_INFO,"USB Connected.");
    }
    dev_BatchUpdate();
    dev_Remote_Reset();
    //vpn_Reset();

    return (res==RET_OK);
}

bool command::gps_default_setting()
{
    string glonass,beidou,gps,qzss,sbas,lband;
    int glonass_dis=0;
    int beidou_dis=0;
    int gps_dis=0;
    int galileo_dis=0;
	int qzss_dis=0;
	int sbas_dis=0;
	int retry;

    if(gpsboard==GPS_TRIMBLE)
    {
        string galileo;
        xmlconfig->getValue("SYSTEM.GPS.GALILEO",&galileo);
        transform(galileo.begin(), galileo.end(), galileo.begin(), ::toupper);
        if(galileo.length()==0)
        {
            galileo=="DISABLE";
            xmlconfig->setValue("SYSTEM.GPS.GALILEO",galileo);
        }
        if(galileo=="DISABLE")
        {
            galileo_dis=1;
        }
        if(galileo=="ENABLE")
        {
            //oem->txtcommand(SET_TRACKING);
        }
        oem->setGalileo(galileo_dis);

        string elem;
        if (xmlconfig->getValue("SYSTEM.GPS.CUTANGLE",&elem)!= RET_OK)
            elem="0";
        oem->setElemask(atoi(elem.c_str()));

        xmlconfig->getValue("SYSTEM.GPS.GLONASS",&glonass);
        transform(glonass.begin(), glonass.end(), glonass.begin(), ::toupper);
        if(glonass=="DISABLE")
            glonass_dis=1;
        oem->setGlonass(glonass_dis);
        xmlconfig->getValue("SYSTEM.GPS.BEIDOU",&beidou);
        transform(beidou.begin(), beidou.end(), beidou.begin(), ::toupper);
        if(beidou=="DISABLE")
            beidou_dis=1;
        oem->setBeidou(beidou_dis);
        xmlconfig->getValue("SYSTEM.GPS.GPS",&gps);
        transform(gps.begin(), gps.end(), gps.begin(), ::toupper);
        if(gps=="DISABLE")
            gps_dis=1;
        oem->setGPS(gps_dis);
		xmlconfig->getValue("SYSTEM.GPS.QZSS",&qzss);
		transform(qzss.begin(), qzss.end(), qzss.begin(), ::toupper);
		if(qzss=="DISABLE")
			qzss_dis=1;
		oem->setQzss(qzss_dis);

		xmlconfig->getValue("SYSTEM.GPS.SBAS",&sbas);
		transform(sbas.begin(), sbas.end(), sbas.begin(), ::toupper);
		if(sbas=="DISABLE")
			sbas_dis=1;
		oem->setSbas(sbas_dis);

		retry=10;
		bool res=false;
		do{
			res=oem->txtcommand(SET_PORT_TCP_5017);
			if(res){
				syslog(LOG_LOCAL7|LOG_INFO,"gps init success.retry:%d",retry);
				break;
			}
			sleep(10);
		}while(0<=retry--);
		if(retry<0)
			syslog(LOG_LOCAL7|LOG_ERR,"gps init failed.retry:%d",retry);
        oem->txtcommand(ADD_PORT_TCP_5020);
        oem->txtcommand(ADD_PORT_TCP_5021);
        oem->txtcommand(ADD_PORT_TCP_5022);
        oem->txtcommand(ADD_PORT_TCP_5023);

        oem->txtcommand(SET_TRACKING);

        string onepps;
        xmlconfig->getValue("SYSTEM.GPS.ONEPPS",&onepps);
        transform(onepps.begin(), onepps.end(), onepps.begin(), ::toupper);
        if(onepps.compare("ENABLE") == 0) oem->txtcommand(ENABLE_1PPS);
        else oem->txtcommand(DISABLE_1PPS);

        string event,slope;
        xmlconfig->getValue("SYSTEM.GPS.EVENT",&event);
        transform(event.begin(), event.end(), event.begin(), ::toupper);
        xmlconfig->getValue("SYSTEM.GPS.EVENT_SLOPE",&slope);
        transform(slope.begin(), slope.end(), slope.begin(), ::toupper);
        if(event.compare("ENABLE") == 0)
        {
            oem->txtcommand(ENABLE_EVENT);
            if(slope.compare("NEGATIVE") == 0) oem->txtcommand(EVENT_NEGATIVE);
            else oem->txtcommand(EVENT_POSITIVE);
        }
        else oem->txtcommand(DISABLE_EVENT);
    }
    else
    {
        if(default_set==0)
        {
            bool res=oem->init();
            if(!res)
            {
                init_gps();
                sleep(1);
                retry=6;
                do{
                    res=oem->init();
                    if(res)
                    {
                        syslog(LOG_LOCAL7|LOG_INFO,"gps init success.");
                        break;
                    }
                    printf("==retry %d==\n",retry);
                    if(retry==2){
                        syslog(LOG_LOCAL7|LOG_WARNING,"Oem Reset !!!");
                        system("rmmod oem_enable 1> /dev/null  2>&1");
                        sleep(1);
                        system("insmod /lib/modules/oem-enable.ko 1> /dev/null  2>&1");
                        sleep(5);
                        init_gps();
                        sleep(3);
                    }
                    sleep(10);
                }while(0<=retry--);
                if(retry<=0)
                {
                    syslog(LOG_LOCAL7|LOG_INFO,"gps init fail.");
                    return  RET_EXEC_FAILED;
                }
            }
			else syslog(LOG_LOCAL7|LOG_INFO,"gps init success.");
            default_set = 1;

			if(gpsboard==GPS_UNICORECOMM)
			{
				int oem_ip_err=0;
				retry=6;
				do{
					oem_ip_err=OEM_Ping();
					if(oem_ip_err==0)
					{
						syslog(LOG_LOCAL7|LOG_INFO,"oem dhcp success: %d.",oem_ip_err);
						break;
					}
					sleep(2);
				}while(--retry);
			}
        }

        string elem;
        if (xmlconfig->getValue("SYSTEM.GPS.CUTANGLE",&elem)!= RET_OK)
            elem="0";
        oem->setElemask(atoi(elem.c_str()));

        xmlconfig->getValue("SYSTEM.GPS.GLONASS",&glonass);
        transform(glonass.begin(), glonass.end(), glonass.begin(), ::toupper);
        if(glonass=="DISABLE")
            glonass_dis=1;
        oem->setGlonass(glonass_dis);

        xmlconfig->getValue("SYSTEM.GPS.BEIDOU",&beidou);
        transform(beidou.begin(), beidou.end(), beidou.begin(), ::toupper);
        if(beidou=="DISABLE")
            beidou_dis=1;
        oem->setBeidou(beidou_dis);

        xmlconfig->getValue("SYSTEM.GPS.GPS",&gps);
        transform(gps.begin(), gps.end(), gps.begin(), ::toupper);
        if(gps=="DISABLE")
            gps_dis=1;
        oem->setGPS(gps_dis);

        string galileo;
        xmlconfig->getValue("SYSTEM.GPS.GALILEO",&galileo);
        transform(galileo.begin(), galileo.end(), galileo.begin(), ::toupper);
        if(galileo.length()==0)
        {
            galileo=="DISABLE";
            xmlconfig->setValue("SYSTEM.GPS.GALILEO",galileo);
        }
        if(galileo=="DISABLE")
        {
            galileo_dis=1;
        }
        oem->setGalileo(galileo_dis);

		xmlconfig->getValue("SYSTEM.GPS.QZSS",&qzss);
		transform(qzss.begin(), qzss.end(), qzss.begin(), ::toupper);
		if(qzss=="DISABLE")
			qzss_dis=1;
		oem->setQzss(qzss_dis);

		xmlconfig->getValue("SYSTEM.GPS.SBAS",&sbas);
		transform(sbas.begin(), sbas.end(), sbas.begin(), ::toupper);
		if(sbas=="DISABLE")
			sbas_dis=1;
		oem->setSbas(sbas_dis);

        if(gpsboard==GPS_HEMISPHERE)
        {
            string atlas;
            if(xmlconfig->getValue("GPS.INFO.FUNCTIONALITY",&atlas)!=RET_OK||atlas.length()==0)
                atlas="";
            if(atlas.find("ATLAS_LBAND")!=std::string::npos)
            {
                if(xmlconfig->getValue("SYSTEM.GPS.LBAND",&lband)!=RET_OK||lband.length()==0)
                    lband="DISABLE";
                transform(lband.begin(), lband.end(), lband.begin(), ::toupper);

                if(lband=="ENABLE"){
                    oem->txtcommand((char *)"$JDIFF,LBAND");
                    oem->txtcommand((char *)"$JDIFFX,INCLUDE,ATLAS");
                }else{
                    oem->txtcommand((char *)"$JDIFF,OTHER");
                }
            }
        }

#ifdef ANTENNA_MANAGEMENT
            initAntennaModel();
#endif
    }
        return RET_OK;
}

int command::ntrip_Reset()
{
    string str,value;
    char tmp[128];
    int i=0,num=0;

    xmlconfig->getValue("NTRIP.MAX_CONNECTION",&value);
    num=atoi(value.c_str());

    //cout<<"num " << value<<endl;
    for(i=0; i<num; i++)
    {
        sprintf(tmp,"%d",i);
        str.assign("NTRIP.CONNECTION@ID:");
        str.append(tmp);
        xmlconfig->getValue(str+".AUTO_CONNECT",&value);
        if(value.compare("YES") == 0 ||value.compare("yes") == 0)
        {
            set_ntrip_connect(i);
        }
    }

    return RET_OK;
}

int command::System()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_EMAIL:
        return sys_Email();
    case CMD_SMS:
        return sys_Sms();
    case CMD_HTTP_PORT:
        return sys_HttpPort();
    case CMD_DEFAULT:
        return sys_Default();
	case CMD_TIMEZONE:
		return sys_Timezone();
	case CMD_TZID:
		return sys_Tzid();
    default:
        return -1;
    }
    return -1;
}

int command::sys_Tzid()
{
	char tmpStr[256]={0};

	if(param.length()==0)
		return RET_ERR_PARAM;

	sprintf(tmpStr,"ln -s /usr/share/zoneinfo/%s /etc/localtime",param.c_str());

	system("rm -f /etc/localtime");
	system(tmpStr);

	return RET_OK;
}

int command::sys_Timezone()
{
	string time_zone;
	string value;

	if(param.length()==0)
		return RET_ERR_PARAM;

	if(xmlconfig->getValue("SYSTEM.TIMEZONE",&value)!= RET_OK)
		value = "8";
	time_zone=value;

	if(time_zone.compare(param) != 0 )
	{
		xmlconfig->setValue("SYSTEM.TIMEZONE",param);
		xmlconfig->saveConfig();

		system("touch /tmp/timezone_conf");

		if((param.find("AUTO"))!=string::npos)
		{
			return RET_OK;
		}
		xmlconfig->setValue("SYSTEM.TIMEZONE_AUTO",param);

		set_sysTimezone(atoi(param.c_str()));

		usleep(100*1000);
		string strSessionName;
		char tmp[128];
		xmlconfig->getList("RECORD.SESSION@NAME",&value);
		std::vector<std::string> x = split(value.c_str(), '|');
		for(int i=0;i<(int)x.size();i++)
		{
			sprintf(tmp,"%s",x.at(i).c_str());

			strSessionName.assign("RECORD.SESSION@NAME:");
			strSessionName.append(tmp);

			xmlconfig->getValue("DATAFLOW."+strSessionName+".STATUS",&value);
			if(value.compare("recording")==0)
			{
				set_record_stop(tmp);
				xmlconfig->setValue("DATAFLOW."+strSessionName+".STATUS","idle");
			}
		}
		xmlconfig->saveRealtime();
	}

	return RET_OK;
}

int command::sys_Email()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_TEST:
        return sys_EmailTest();
    default:
        return -1;
    }
    return -1;
}
int command::sys_Sms()
{
    enPathField enpath =(enPathField)node[2];
    //printf("Device %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_TEST:
        return sys_SmsTest();
    default:
        return -1;
    }
    return -1;
}

int command::sys_HttpPort()
{
    int port=atoi(param.c_str());
    if(port>0 && port <65535)
    {
        xmlconfig->setValue("SYSTEM.HTTP_PORT",param);
        xmlconfig->saveConfig();
        sync();
        string cmd;
        cmd="echo \"Listen "+param+"\"> "+HTTP_PORT_PATH;
         pid_t pid;
        pid=fork();
        if (pid == -1)
        {
            syslog(LOG_LOCAL7|LOG_ERR,"sys_HttpPort()  fork() error!");
            return RET_EXEC_FAILED;
        }
        if (pid == 0)
        {
            system(cmd.c_str());
            system("killall httpd 1> /dev/null 2>&1");
            system("/opt/apache/bin/apachectl start");
            //cout<<cmd<<endl;
            exit(0);
        }
    }
    else
    {
        return RET_ERR_PARAM;
    }
    return RET_OK;
}

int  command::sys_Default()
{
    gps_default_setting();

    return RET_OK;
}

int command::sys_EmailTest()
{
    return send_email("test","This is a test email.");
}

int command::sys_SmsTest()
{
    return send_sms("This is a test sms.");
}

int command::do_SET(int argc, string (&argv)[MAX_CMD_ARGS])
{
    int result;
    unsigned int index;
    unsigned int start,end;
    string line=argv[1];
    string word;
    index = 0;
    nodes=0;
    transform(line.begin(), line.end(), line.begin(), ::toupper);
    //cout<<"line : " + line <<endl;
    while ((nodes < MAX_CMD_NODE) && (index < line.length()))
    {
        start = index;
        while ((index < line.length()) && ((line[index] != '.')))
        {
            index++;
        }
        end = index;
        word = line.substr(start,end - start);
        node[nodes]=GetEnumFromKeyword((char *)word.c_str());
        //cout<<"node : " + word <<endl;
        //printf("enpath[%d]: %d\n",nodes,node[nodes]);
        nodes++;
        index++;
    }
    //printf("nodes: %d\n",nodes);
    if(nodes<2)
    {
        return RET_INVALID_NODE;
    }
    param.clear();
    if(argv[2].length()) param=argv[2];
    result=Dowork();
    if(result==-1)
    {
        result = xmlconfig->setValue(argv[1],argv[2]);
    }
    if (result == RET_OK)
    {
        if(argc==1)
            ShowMsgLine(argv[0] + ","+ argv[1]+ ",OK" );
        else
            ShowMsgLine(argv[0] + ","+ argv[1]+ ","+ argv[2]+ ",OK" );
    }
    return result;

}
