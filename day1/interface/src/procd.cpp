#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <list>
#include "share.hpp"
#include "procd.hpp"
#include "util.hpp"

using namespace std;

static int sat;
static float pdop;
static int ready=0;

typedef struct _PROCD_FUNC
{
    int  interval;//sec
    unsigned int  nexttick;
    int options;
    unsigned int reserve1;
    unsigned int reserve2;
    void (*Fuction)(int ,command *);
} PROCD_FUNC;

#define Func_Num  (sizeof(Func)/sizeof(PROCD_FUNC))

void procd_rec(int n, command *cmd);
void procd_ntrip(int n, command *cmd);
void procd_init(int n, command *cmd);
void procd_memory(int n, command *cmd);
void procd_monitorPower(int n, command *cmd);
void procd_monitorGPS(int n, command *cmd);
void procd_monitorRunTime(int n, command *cmd);
void procd_monitorCheckRegiDate(int n, command *cmd);
//void procd_monitorRawEph(int n, command *cmd);

PROCD_FUNC Func[]=
{
    {3,0,0,0,0,procd_rec},
    {2,0,0,0,0,procd_ntrip},
    {2,0,0,0,0,procd_init},
    {10,0,0,0,0,procd_memory},
    {5,0,0,0,0,procd_monitorPower},
    {10,0,0,0,0,procd_monitorGPS},
    {60,0,0,0,0,procd_monitorRunTime},
    {15,0,0,0,0,procd_monitorCheckRegiDate},
};

static unsigned int msg_counter=0;
static unsigned int error_counter=0;
void procd_monitorRunTime(int n, command *cmd)
{
    char tmp[128];
    string value;
    unsigned int t;
    unsigned int v;
    time_t now ;

//Func[n].reserve1 start time
//Func[n].reserve2 last time
    now =time(0);
    t=now/60;
    if(Func[n].options)
    {
        v = t-Func[n].reserve1;
        //syslog(LOG_LOCAL7|LOG_INFO,"run time diff %d, %d - %d",v,t,Func[n].reserve2);

        if((t-Func[n].reserve2)>10)//磁盘操作频繁时此处时差会比较大
        {
            Func[n].reserve1=t-1;
            v=1;
        }
        Func[n].reserve2=t;

        sprintf(tmp,"%d",v);
        value.assign(tmp);
        cmd->xmlconfig->setValue("SYSTEM.RUNTIME",value);
        cmd->xmlconfig->saveConfig();
		//cout<<"#### runtime "<<value<<endl;

        //gps error
        if( (cmd->oem_msg_counter==msg_counter) || (cmd->oem->error_count>5))
        {
            if(v>30 && cmd->direct_link==0)//30min
            {
                if((error_counter>1) || (cmd->gpsboard==GPS_TRIMBLE))
                {
                    syslog(LOG_LOCAL7|LOG_ERR,"!!!!!!!!!! IMPORTANT !!!!!!!!!!");
                    syslog(LOG_LOCAL7|LOG_ERR,"Oem communication error, system will reboot!");
                    cmd->dev_Reset();
                }
                else
                {
                    //reset gps
                    syslog(LOG_LOCAL7|LOG_ERR,"!!!!!!!!!! IMPORTANT !!!!!!!!!!");
                    syslog(LOG_LOCAL7|LOG_ERR,"Oem communication error, will reset it.");
                    system("rmmod oem_enable 1> /dev/null  2>&1");
                    sleep(1);
                    system("insmod /lib/modules/oem-enable.ko 1> /dev/null  2>&1");
                    error_counter++;

                    ///cmd->init_gps();
                }
            }
        }
        else
        {
            error_counter=0;
        }
        msg_counter=cmd->oem_msg_counter;

    }
    else
    {
        Func[n].reserve1=t-1;
        Func[n].reserve2=t;
        Func[n].options=1;
    }
}

//if transmitting then disconnet
void stop_ntrip(command *cmd)
{
    string value;

    cmd->xmlconfig->getValue("DATAFLOW.NTRIP.CONNECTION@ID:0.STATUS",&value);
    if(value.compare("transmitting") == 0)
        cmd->set_ntrip_disconnect(0);

    cmd->xmlconfig->getValue("DATAFLOW.NTRIP.CONNECTION@ID:1.STATUS",&value);
    if(value.compare("transmitting") == 0)
        cmd->set_ntrip_disconnect(1);

    cmd->xmlconfig->getValue("DATAFLOW.NTRIP.CONNECTION@ID:2.STATUS",&value);
    if(value.compare("transmitting") == 0)
        cmd->set_ntrip_disconnect(2);

    cmd->xmlconfig->getValue("DATAFLOW.NTRIP.CONNECTION@ID:3.STATUS",&value);
    if(value.compare("transmitting") == 0)
        cmd->set_ntrip_disconnect(3);
}

void stop_all(command *cmd)
{
    string value;
    int flag=0;
    cmd->xmlconfig->getValue("DATAFLOW.RECORD.STATUS",&value);
    if(value.compare("stop") != 0){
        cmd->param="";
        cmd->dev_rec_Stoprec();
    }

    if(cmd->base_started==1)
        cmd->dev_StopBase();

    cmd->xmlconfig->getValue("PORTS.BLUETOOTH.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.BLUETOOTH.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.RADIO.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.RADIO.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.COM1.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.COM1.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.COM2.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.COM2.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.COM3.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.COM3.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.NTRIP_CLIENT.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.NTRIP_CLIENT.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.SOCKET.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.SOCKET.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.SOCKET1.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.SOCKET1.ENABLE","NO");
        flag = 1;
    }

    cmd->xmlconfig->getValue("PORTS.SOCKET2.ENABLE",&value);
    if(value.compare("NO") != 0 )
    {
        cmd->xmlconfig->setValue("PORTS.SOCKET2.ENABLE","NO");
        flag = 1;
    }

#ifndef CN_VERSION
    cmd->xmlconfig->getValue("NETWORK.WIFI.ENABLE",&value);
    if(value.compare("NO"))
    {
        cmd->xmlconfig->setValue("NETWORK.WIFI.ENABLE","NO");
        cmd->xmlconfig->saveConfig();
        cmd->net_wifi_Reset();
    }

    cmd->xmlconfig->getValue("NETWORK.GPRS.ENABLE",&value);
    if(value.compare("NO"))
    {
        cmd->xmlconfig->setValue("NETWORK.GPRS.ENABLE","NO");
        cmd->xmlconfig->saveConfig();
        cmd->net_gprs_Reset();
    }
#endif

    if(flag==1)
    {
        cmd->xmlconfig->saveConfig();
        cmd->port_Reset();
    }
    //cmd->ntrip_disconnect();
    stop_ntrip(cmd);
}


void procd_monitorCheckRegiDate(int n, command *cmd)
{
    string value;
    char tmp[10];
#ifdef CheckRegi
if(cmd->checkreg==1)
{
    if(cmd->SysParam.reg_state==REG_ERROR)
    {
        //cmd->dev_rec_Stoprec();
        //cmd->dev_StopBase();
        //stop_ntrip(cmd);
        stop_all(cmd);
        syslog(LOG_LOCAL7|LOG_INFO,"IMPORTANT: Register code is ERROR. \n");
        return;
    }

    cmd->xmlconfig->getValue("GPS.TIME.GPSDATE",&value);
    if(value.length()==10)
    {
        strcpy(tmp,value.c_str());
        if(tmp[0]>'9'||tmp[0]<'0') return;
        cmd->SysParam.NowDateTime=(tmp[0]-'0')*10000000;
        if(tmp[1]>'9'||tmp[1]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[1]-'0')*1000000;
        if(tmp[2]>'9'||tmp[2]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[2]-'0')*100000;
        if(tmp[3]>'9'||tmp[3]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[3]-'0')*10000;
        if(tmp[5]>'9'||tmp[5]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[5]-'0')*1000;
        if(tmp[6]>'9'||tmp[6]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[6]-'0')*100;
        if(tmp[8]>'9'||tmp[8]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[8]-'0')*10;
        if(tmp[9]>'9'||tmp[9]<'0') return;
        cmd->SysParam.NowDateTime+=(tmp[9]-'0');
        //printf("=====SysParam.NowDateTime = %u\n",cmd->SysParam.NowDateTime);
        //syslog(LOG_LOCAL7|LOG_INFO,"=====SysParam.NowDateTime = %u\n", cmd->SysParam.NowDateTime);
        //syslog(LOG_LOCAL7|LOG_INFO,"=====SysParam.ExpireDateTime = %u\n", cmd->SysParam.ExpireDateTime);

        if(cmd->SysParam.NowDateTime<=20000000)
        {
            cmd->xmlconfig->setValue("SYSTEM.STATE","CHECKING");
            cmd->xmlconfig->setValue("DEVICE.STATUS","0111");
            cmd->SysParam.reg_state=REG_CHECKING;
            cmd->xmlconfig->saveConfig();
            if(Func[n].interval!=10)
            {
                Func[n].interval=10;
            }
            syslog(LOG_LOCAL7|LOG_INFO,"IMPORTANT: Register code is CHECKING. \n");
        }
        else if(cmd->SysParam.NowDateTime<=cmd->SysParam.ExpireDateTime)
        {
            cmd->xmlconfig->setValue("SYSTEM.STATE","NORMAL");
            cmd->xmlconfig->setValue("DEVICE.STATUS","0");
            cmd->SysParam.reg_state=REG_OK;
            cmd->xmlconfig->saveConfig();
#ifndef CN_VERSION
            if(!(cmd->SysParam.DeviceOption&0x40))
            {
                cmd->xmlconfig->getValue("NETWORK.WIFI.ENABLE",&value);
                if(value.compare("NO"))
                {
                    cmd->xmlconfig->setValue("NETWORK.WIFI.ENABLE","NO");
                    cmd->xmlconfig->saveConfig();
                    cmd->net_wifi_Reset();
                }
            }
            if(!(cmd->SysParam.DeviceOption&0x10))
            {
                cmd->xmlconfig->getValue("NETWORK.GPRS.ENABLE",&value);
                if(value.compare("NO"))
                {
                    cmd->xmlconfig->setValue("NETWORK.GPRS.ENABLE","NO");
                    cmd->xmlconfig->saveConfig();
                    cmd->net_gprs_Reset();
                }
            }
#endif
            /*注册以后 60s检查一次*/
            if(Func[n].interval!=60)
            {
                Func[n].interval=60;
            }
            //syslog(LOG_LOCAL7|LOG_INFO,"IMPORTANT: Register code is OK. \r\n");
        }
        else
        {
            cmd->SysParam.reg_state=REG_EXPIRED;
            cmd->xmlconfig->setValue("SYSTEM.STATE","EXPIRED");
            cmd->xmlconfig->setValue("DEVICE.STATUS","0111");
            cmd->xmlconfig->saveConfig();

            stop_all(cmd);
            if(Func[n].interval!=10)
            {
                Func[n].interval=10;
            }
            //printf("IMPORTANT: Register code is expired.\n");
            syslog(LOG_LOCAL7|LOG_INFO,"IMPORTANT: Register code is expired. \n");
        }
    }else
            Func[n].interval=10;
}
#endif
}

bool alert(command *cmd,string msg)
{
    string value;
    string msg1,msg2,t;
    get_time(&t);
    cmd->xmlconfig->getValue("DEVICE.INFO.SERIAL",&msg1);
    msg2=msg+" "+msg1+"@"+t;
    cmd->xmlconfig->getValue("SYSTEM.EMAIL_ALERT",&value);
    if(value.compare("YES") == 0 ||value.compare("yes") == 0)
    {
        cmd->send_email(msg1,msg2);
    }
    cmd->xmlconfig->getValue("SYSTEM.SMS_ALERT",&value);
    if(value.compare("YES") == 0 ||value.compare("yes") == 0)
    {
        cmd->send_sms(msg2);
    }
    return true;
}

void procd_rec(int n, command *cmd)
{
    char tmp[128];
    string str,value;
    string name;

   // if((cmd->SysParam.reg_state==REG_EXPIRED)||(cmd->SysParam.reg_state==REG_ERROR))
    //    return;
    //printf("sat %d\n",sat);
	if(!ready) return;
    if(sat<4  || pdop>6) return;
    if(cmd->usb_connected==1)  return;

    cmd->xmlconfig->getList("RECORD.SESSION@NAME",&value);

//    char *strValue[20];
//    int i,num;
    //num = _split(value.c_str(),'|',strValue,20);
//    for(i = 0 ; i < num ; i ++)
//    {
//        sprintf(tmp,"%s",strValue[i]);
//        str.assign("RECORD.SESSION@NAME:");
//        str.append(tmp);
//    }

    std::vector<std::string> x = split(value.c_str(), '|');
    //syslog(LOG_LOCAL7|LOG_INFO,"reserve1:%d==> %s %d",Func[n].reserve1,value.c_str(),x.size());
    for(int i=0;i<(int)x.size();i++)
    {
        sprintf(tmp,"%s",x.at(i).c_str());
        str.assign("RECORD.SESSION@NAME:");
        str.append(tmp);

        cmd->xmlconfig->getValue("DATAFLOW."+str+".STATUS",&value);
        //syslog(LOG_LOCAL7|LOG_INFO,"GET %s  : %s",str.c_str(),value.c_str());
		if(value.find("idle")!=std::string::npos||value.find("IDLE")!=std::string::npos)
        {
            if(Func[n].reserve1++<(2*((unsigned int)i+1))) return;
//            if(cmd->record_enable)
//            {
//                cmd->set_record_stop(tmp);
//            }

            cmd->xmlconfig->getValue(str+".AUTO_REC",&value);
            //syslog(LOG_LOCAL7|LOG_INFO,"AUTO_REC %s  : %s",str.c_str(),value.c_str());
            if(value.compare("YES") == 0 ||value.compare("yes") == 0)
            {
                int ret = cmd->set_record_start(tmp);
                if(ret)
                {
                    syslog(LOG_LOCAL7|LOG_ERR,"auto start record [%s] error:%d",tmp,ret);
                }
                sleep(1);
            }
        }
    }
    Func[n].reserve1=0;

    cmd->xmlconfig->getValue("DATAFLOW.RECORD.SESSION@NAME:*.STATUS",&value);
    if(!strstr(value.c_str(),"recording"))
    {
        cmd->xmlconfig->setValue("DATAFLOW.RECORD.STATUS","idle");
    }else{
        cmd->xmlconfig->setValue("DATAFLOW.RECORD.STATUS","recording");
    }


#if 0
    cmd->xmlconfig->getValue("DATAFLOW.RECORD.STATUS",&value);
    if(value.compare("idle") == 0||value.compare("IDLE") == 0)
    {
        if(Func[n].options) return;
        if(Func[n].reserve1++<3) return;
        if(cmd->record_enable)
        {
            cmd->dev_rec_Stoprec();
        }
        cmd->xmlconfig->getValue("RECORD.AUTO_REC",&value);
        if((value.compare("YES") == 0 ||value.compare("yes") == 0) /*&& (cmd->record_enable==0)*/)
        {
            if(cmd->dev_rec_Startrec())
            {
                //error
                //Func[n].options=1;
                Func[n].options=0;
                syslog(LOG_LOCAL7|LOG_ERR,"auto start record error");
            }
            else
            {
                Func[n].options=0;
            }
            Func[n].reserve1=0;
        }
    }
    else
    {
        Func[n].reserve1=0;//
    }
#endif
    //printf("rec tick %ld %d\n",time(NULL),sizeof(unsigned long int));
}


void procd_ntrip(int n, command *cmd)
{
    char tmp[128];
    string str,value;
    int i,num;

	if(!ready) return;
    if(sat<4  || pdop>6) return;
    if(Func[n].options) return;

    cmd->xmlconfig->getValue("NTRIP.MAX_CONNECTION",&value);
    num=atoi(value.c_str());

    //cout<<"num " << value<<endl;
    for(i=0; i<num; i++)
    {
        sprintf(tmp,"%d",i);
        str.assign("NTRIP.CONNECTION@ID:");
        str.append(tmp);
        cmd->xmlconfig->getValue("DATAFLOW."+str+".STATUS",&value);
        //cout<<str <<"status " << value<<endl;
        if(value.compare("idle") == 0||value.compare("IDLE") == 0)
        {
            //if(Func[n].reserve1++<3) break;
            //else Func[n].reserve1=0;
            cmd->xmlconfig->getValue(str+".AUTO_CONNECT",&value);
            if(value.compare("YES") == 0 ||value.compare("yes") == 0)
            {
				int ret = cmd->set_ntrip_connect(i);
                if(ret)
                {
                    //error
                    //Func[n].options=1;
					syslog(LOG_LOCAL7|LOG_ERR,"auto start ntrip [%d] error:%d (%s)",i,ret,APP_VERSION);
                }
                else
                {
                    Func[n].options=0;

                }
                sleep(1);
            }
        }
        else
        {
            Func[n].reserve1=0;
        }
    }
    //printf("rec tick %ld %d\n",time(NULL),sizeof(unsigned long int));
}


void procd_init(int n, command *cmd)
{
    if(Func[n].options>2)  return;

    Func[n].options++;
    if(Func[n].options==2)
    {
        //printf("here\n");
#ifdef DEBUG
#else
        cmd->default_setting();
		ready=1;
#endif
    }
}
typedef struct RecFileInfo
{
	char * szFileName;
	unsigned long long ulFileSize;
} REC_FILE_INFO;
//check all running record process storage pool status
//unsigned long long CheckRecordSessionSize(command *cmd)
void CheckRecordSessionSize(command *cmd)
{
	string value;
	//printf("CheckRecordSessionSize begin\n");

	cmd->xmlconfig->getList("RECORD.SESSION@NAME",&value);//get all session name split "|"
	std::vector<std::string> x = split(value.c_str(), '|');

	for(int iSessionIndex=0;iSessionIndex<(int)x.size();iSessionIndex++)
	{
		string str;
		string str2;
		char szSessionName[128]={0};
		unsigned long long ullsize=0;
		char szSessionFileLogName[MAX_PATH];

		sprintf(szSessionName,"%s",x.at(iSessionIndex).c_str());

		str2.assign("DATAFLOW.RECORD.SESSION@NAME:");
		str2.append(szSessionName);
		cmd->xmlconfig->getValue(str2+".STATUS",&value);
		//printf("CheckRecordSessionSize %s status %s\n",szSessionName,value.c_str());
		if(!strstr(value.c_str(),"recording"))
		{
			continue;
		}

		str.assign("RECORD.SESSION@NAME:");
		str.append(szSessionName);

		sprintf(szSessionFileLogName,SESSION_FILE_LOG_DIR"%s.txt",szSessionName);

		ullsize = 0;
		list <REC_FILE_INFO *> lstRecordFiles;
		unsigned long long ullSizeLimit=2713563;
		//get size limit from session name
		cmd->xmlconfig->getValue(str+".POOLSIZE",&value);//limit (Mb)
		ullSizeLimit=atoi(value.c_str())*1024ull*1024;
		//printf("CheckRecordSessionSize session filename %s limitsize %llu(%sMb) %d\n",szSessionFileLogName,ullSizeLimit,value.c_str(),atoi(value.c_str()));
		int iLimitType=0;
		//get limit type from session name,0 stop record when full, 1 remove earliest file when full
		cmd->xmlconfig->getValue(str+".POOLMODE",&value);//type/mode  <Off 、Stop、Delete >
		if(value.compare("Stop")==0)
		{
			iLimitType=0;
		}
		else if(value.compare("Delete")==0)
		{
			iLimitType=1;
		}
		else
		{
			continue;
		}

		char szRecordFileName[MAX_PATH];
		unsigned long long ulCurRecordSize=0;
		char szCurRecordFileName[MAX_PATH];

		char buff[513];
		int i;
		int n;
		int k;
		char *p;
		char *pNewLine;
		int rewritelist=0;
		printf("CheckRecordSessionSize session filename %s limitsize %llu\n",szSessionFileLogName,ullSizeLimit);
		int iReadModifyTime=0;
		struct stat statLog1;
		char modify_time[50];
		if(stat(szSessionFileLogName,&statLog1)==0)
		{
			iReadModifyTime++;
		}
    	strftime(modify_time, 50, "%Y-%m-%d %H:%M:%S", localtime(&statLog1.st_mtime));
		int fileSessionFileLog=open(szSessionFileLogName,O_RDONLY, 0666);
		if (fileSessionFileLog<0)
		{
			//return ullsize;
			continue;
		}

		//get file name and size from  session name xmlconfig size
		cmd->xmlconfig->getValue("DATAFLOW."+str+".PATH",&value);//filename with absolute path
		sprintf(szCurRecordFileName,RECORD_PATH"%s",value.c_str());

		cmd->xmlconfig->getValue("DATAFLOW."+str+".SIZE",&value);//size from  session (Bytes)
		ulCurRecordSize=atoi(value.c_str());

		printf("CheckRecordSessionSize curinfo %s -- %llu,modify time %s \n",szCurRecordFileName,ulCurRecordSize,modify_time);
		k=0;
		while((i = read(fileSessionFileLog,buff,sizeof(buff)-1)) > 0)
		{
			buff[i]='\0';
			p=buff;
			//printf("[%d]->buf:%s||\n",i,buff);
			while ((pNewLine=strchr(p,'\n'))!=NULL)
			{
				n=pNewLine-p;
				//if (n>0)
				if (n>=0)
				{
					strncpy(&szRecordFileName[k],p,n);
					szRecordFileName[k+n]='\0';
					list<REC_FILE_INFO *>::iterator it = lstRecordFiles.begin();
					for (; it != lstRecordFiles.end(); ++it)
					{
						if (strcmp(szRecordFileName,(*it)->szFileName)==0)
						{
							break;
						}
					}
					if(it != lstRecordFiles.end())//exist in list, skip
					{
						rewritelist=1;
						printf("dumplicated file in list : %s\n",szRecordFileName);
						goto LBL_NEXT_LINE;
					}

					//printf("list n=%d k=%d:%s--\n",n,k,szRecordFileName);
					//printf("list : %s\n",szRecordFileName);
					if (strcmp(szCurRecordFileName,szRecordFileName)==0)
					{
						REC_FILE_INFO * pTmpInfo= (REC_FILE_INFO*)malloc(sizeof(REC_FILE_INFO));
						pTmpInfo->szFileName = (char*)malloc((strlen(szRecordFileName)+1) *sizeof(char));
						strcpy(pTmpInfo->szFileName,szRecordFileName);
						pTmpInfo->ulFileSize=ulCurRecordSize;
						lstRecordFiles.push_back(pTmpInfo);
						ullsize+=ulCurRecordSize;
					}
					else
					{

						//unsigned long filesize = 0;
						struct stat64 statbuff;
						/*char szRecordFullFileName[MAX_PATH];
						sprintf(szRecordFullFileName,RECORD_PATH"%s",szRecordFileName);*/

						int iTmp=stat64(szRecordFileName, &statbuff);
						//printf("stat %s--%d\n",szRecordFileName,iTmp);
						if( iTmp< 0)
						{
							//skip not exists file
							rewritelist=1;
							printf("CheckRecordSessionSize stat %s failed %d\n",szRecordFileName,iTmp);
						}else{
							REC_FILE_INFO * pTmpInfo=(REC_FILE_INFO*) malloc(sizeof(REC_FILE_INFO));
							pTmpInfo->szFileName =(char*) malloc((strlen(szRecordFileName)+1) *sizeof(char));
							strcpy(pTmpInfo->szFileName,szRecordFileName);
							pTmpInfo->ulFileSize=statbuff.st_size;
							lstRecordFiles.push_back(pTmpInfo);
							ullsize+=statbuff.st_size;
						}
					}

				}
LBL_NEXT_LINE:
				p=pNewLine+1;
				k=0;
			}
			if (*p!='\0')
			{
				k=strlen(p);
				strcpy(szRecordFileName,p);
			}
		}
		close(fileSessionFileLog);
		//printf("CheckRecordSessionSize total file size:%llu\n",ullsize);
		if (ullsize> ullSizeLimit)
		{
			unsigned long long ullExceedSize=ullsize-ullSizeLimit;
			if (iLimitType==0)
			{
				//stop record when full
				//stop record process session name
				printf("CheckRecordSessionSize stop record\n");
				cmd->set_record_stop(szSessionName);
			}else{
				int bDeleteCurRecord=0;
				//remove earliest file when full
				list<REC_FILE_INFO *>::iterator it = lstRecordFiles.begin();
				for (; it != lstRecordFiles.end(); ++it)
				{
					//char szRecordFullFileName[MAX_PATH];
					//sprintf(szRecordFullFileName,"%s%s",RECORD_PATH,(*it)->szFileName);
					//remove(szRecordFullFileName);
					if (strcmp(szCurRecordFileName,(*it)->szFileName)==0)
					{
						//to delete szCurRecordFileName,first stop record
						cmd->set_record_stop(szSessionName);
						bDeleteCurRecord=1;
					}
					remove((*it)->szFileName);
					//printf("CheckRecordSessionSize remove file %s ,filesize %llu, ullExceedSize=%llu  \n",(*it)->szFileName,(*it)->ulFileSize,ullExceedSize);
					if((p=strrchr((*it)->szFileName,'/'))>0)
					{
						char fileDir[256]={0};
						strncpy(fileDir,(*it)->szFileName,p-(*it)->szFileName+1);
						if(isempty(fileDir))remove(fileDir);
					}

					if (ullExceedSize< (*it)->ulFileSize)
					{
						break;
					}else{
						ullExceedSize-= (*it)->ulFileSize;
					}
				}
				struct stat statLog2;
				if(stat(szSessionFileLogName,&statLog2)==0)
				{
					iReadModifyTime++;
					double seconds = difftime(statLog1.st_mtime, statLog2.st_mtime);
  					if (seconds != 0) {
  						char modify_time2[50];
  						strftime(modify_time2, 50, "%Y-%m-%d %H:%M:%S", localtime(&statLog2.st_mtime));
  						printf("CheckRecordSessionSize 111 session %s -- read time %s,modify time %s, time changed! cancel rewriting!!! \n",szSessionFileLogName,modify_time,modify_time2);
  						goto LBL_CLOSE_REWRITE1;
  					}
				}
				fileSessionFileLog = open(szSessionFileLogName, O_RDWR|O_CREAT|O_TRUNC, 0666);
				if (fileSessionFileLog>0)
				{

					it++;
					for (; it != lstRecordFiles.end(); ++it)
					{
						char szRecordFullFileName[MAX_PATH+4];
						strcpy(szRecordFullFileName,(*it)->szFileName);
						strcat(szRecordFullFileName,"\n");
						write(fileSessionFileLog,szRecordFullFileName,strlen(szRecordFullFileName));
						//printf("rewrite list 111:%s",szRecordFullFileName);
					}
					fsync(fileSessionFileLog);
					close(fileSessionFileLog);
				}
LBL_CLOSE_REWRITE1:

				if (bDeleteCurRecord!=0)
				{
					//restart record after deleting szCurRecordFileName,this should do after close fileSessionFileLog
					cmd->set_record_start(szSessionName);
				}

			}
		}
		else
		{
			if(rewritelist)
			{
				struct stat statLog2;
				if(stat(szSessionFileLogName,&statLog2)==0)
				{
					iReadModifyTime++;
					double seconds = difftime(statLog1.st_mtime, statLog2.st_mtime);
  					if (seconds != 0) {
  						char modify_time2[50];
  						strftime(modify_time2, 50, "%Y-%m-%d %H:%M:%S", localtime(&statLog2.st_mtime));
  						printf("CheckRecordSessionSize 222 session %s -- read time %s,modify time %s, time changed! cancel rewriting!!! \n",szSessionFileLogName,modify_time,modify_time2);
  						goto LBL_CLOSE_REWRITE2;
  					}
				}
				fileSessionFileLog = open(szSessionFileLogName, O_RDWR|O_CREAT|O_TRUNC, 0666);
				if (fileSessionFileLog>0)
				{
					list<REC_FILE_INFO *>::iterator it = lstRecordFiles.begin();
					for (; it != lstRecordFiles.end(); ++it)
					{
						char szRecordFullFileName[MAX_PATH+4];
						strcpy(szRecordFullFileName,(*it)->szFileName);
						strcat(szRecordFullFileName,"\n");
						//printf("rewrite list:%s\n",szRecordFullFileName);
						write(fileSessionFileLog,szRecordFullFileName,strlen(szRecordFullFileName));

					}
					fsync(fileSessionFileLog);
					close(fileSessionFileLog);
					//printf("CheckRecordSessionSize curinfo %s -- rewrite time %s \n",szCurRecordFileName,modify_time);
				}
LBL_CLOSE_REWRITE2:
				;
			}
		}
		while(!lstRecordFiles.empty())
		{
			list<REC_FILE_INFO *>::iterator it = lstRecordFiles.begin();
			free((*it)->szFileName);
			free(*it);
			lstRecordFiles.pop_front();
		}
	}
	//printf("CheckRecordSessionSize end\n");
	//return ullsize;

}

//如果数据记录写磁盘很频繁(如100hz)，以下操作可能将会很耗时
void procd_memory(int n, command *cmd)
{
#ifdef DEBUG
#else
    string value;
    string size,free;

	if(Func[n].reserve1++==0||Func[n].reserve1%6==0)//1 min
	{
		/*struct timeval start;
		gettimeofday( &start,NULL );
		unsigned  long long timeuse = (1000000 * ( start.tv_sec) + start.tv_usec)/1000 ;
		printf("read disk start: @ %llu\n",timeuse);*/

		get_disk_size("/",&size,&free);
		cmd->xmlconfig->setValue("DEVICE.INTERNAL_MEMORY.TOTAL", size);
		cmd->xmlconfig->setValue("DEVICE.INTERNAL_MEMORY.FREE", free);
		get_disk_size(RECORD_DISK,&size,&free);
		cmd->xmlconfig->setValue("DEVICE.DATA_MEMORY.TOTAL", size);
		cmd->xmlconfig->setValue("DEVICE.DATA_MEMORY.FREE", free);

		unsigned long long ullsize=strtoull(free.c_str (),NULL,10);

		if(ullsize>0 && ullsize<(10*1024*1024))
		{
			if(Func[n].options)
			{
				alert(cmd,"The data memory is full!");
				Func[n].options=0;
			}
		}
		else if(ullsize>0)
		{
			Func[n].options=1;
		}

		//if(ullsize>=0 && ullsize<(500*1024*1024))
		//{
		//    cmd->xmlconfig->getValue("SYSTEM.AUTO_CLEAN",&value);
		//    if(value.compare("YES") == 0||value.compare("yes") == 0)
		//    {
		//        clean_process();
		//    }
		//}
	}
	cmd->xmlconfig->saveConfig();

#endif
}

void procd_monitorPower(int n, command *cmd)
{
    string value;
    string msg1,msg2,t;
    //External power is disconnected
    cmd->xmlconfig->getValue("DEVICE.POWER_SOURCE",&value);
    if(Func[n].options)
    {
        if(value.compare("BATTERY") == 0)
        {
            Func[n].options=0;
            alert(cmd,"External power is disconnected!");
        }
    }
    else
    {
        if(value.compare("EXTERNAL") == 0) Func[n].options=1;
    }
}

void procd_monitorGPS(int n, command *cmd)
{
    int m;
    string value;
    string msg1,msg2,t;

    //External power is disconnected
    cmd->xmlconfig->getValue("GPS.SATELITES.ALL",&value);
    m=atoi(value.c_str());
    if(Func[n].options)
    {
        if(m>0 && m<4)
        {
            Func[n].options=0;
            alert(cmd,"Abnormal GPS signal!");
        }
    }
    else
    {
		if(m>5){
			Func[n].options=1;
			cmd->ntp_enable=1;
			cmd->net_ntp_Reset();
		}
    }

}

void get_gps_data(command *cmd)
{
    string value;
    cmd->xmlconfig->getValue("GPS.SATELITES.ALL",&value);
    sat=atoi(value.c_str());
    cmd->xmlconfig->getValue("GPS.DOP.PDOP",&value);
    pdop=atof(value.c_str());
}

void *procd_thread(void *p)
{
    unsigned int i;
    unsigned int count=0;
    sleep(1);
    while(1)
    {
        get_gps_data((command *)p);
        for(i=0; i<Func_Num; i++)
        {

            if(count>=Func[i].nexttick)
			{
                Func[i].Fuction(i, (command *)p);
                Func[i].nexttick=count+Func[i].interval;
            }
        }
        count++;
        sleep(1);
    }
    return 0;
}

void *pool_thread(void *p)
{
    while(1)
    {
        CheckRecordSessionSize((command *)p);

        sleep(10);
    }

    return 0;
}

void procd(command *cmd)
{
//#ifdef DEBUG
//#else
    pthread_t thread;
    if(pthread_create(&thread, NULL, procd_thread, cmd)!=0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"procd() pthread create error");
        //exit(3);
    }

    pthread_t thread_pool;
    if(pthread_create(&thread_pool, NULL, pool_thread, cmd)!=0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"pool_thread() pthread create error");
        //exit(3);
    }
//#endif
}
