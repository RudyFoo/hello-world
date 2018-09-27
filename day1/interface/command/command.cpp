/*
 * command.cpp
 *
 */


#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <cctype>
#include <algorithm>
#include <sys/msg.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include "command.hpp"
#include "xmlconf.hpp"
#include "tinyxml.h"
#include "util.hpp"
#include "IPCInfoUtil.h"


TXT_CMD command::CmdTable[] =
{
    {"SET", 1, 3, &command::do_SET, "Setup configuration"},
    {"GET", 1, 3, &command::do_GET, "Read configuration"},
    {"LIST", 1, 3, &command::do_LIST, "List configuration"},
    {"SAVE", 0, 0, &command::do_SAVE, "Save configuration to disk"},
    {"UPDATE", 0, 0, &command::do_UPDATE, "Update realtime configuration"},
    {"GETALL", 0, 1, &command::do_GETALL, "List all configuration"},
    {"DEL", 1, 3, &command::do_DEL, "Delete node"},
    {"ADD", 1, 3, &command::do_ADD, "Add node"},
};

command::command(int gps)
{
	xmlconfig = new config();
	int i;
	for(i=0;i<MAX_PROCESS;i++) process[i]=0;
    process[0]=getpid();
    gpsboard=gps;
    oem_msg_counter=0;
    bluetooth_enable=0;
    wifi_enable=0;
    record_enable=0;
    ntrip_connection=0;
    base_started=0;
    DisableSecurityMode=0;
    use_raw_port=0;
    direct_link=0;
    ntrip_double=0;
    recSessionEmpty=1;
	littlegprsboard=0;
    SysParam.hardver=1;
	/*m_IPCRadioCmdRead.iIPCMId=INVALID_IPC_ID;
	m_IPCRadioCmdWrite.iIPCMId=INVALID_IPC_ID;	*/
    get_infos();
#if 1
    //if(gpsboard==0)
    //{
        string strgps;
        xmlconfig->getValue("DEVICE.INFO.GPSBOARD",&strgps);//strgps="OEM729";
        if(strgps=="NOVATEL"||strgps=="OEM628"||strgps=="OEM628E"||strgps=="OEM719"||strgps=="OEM729") gpsboard=GPS_NOVATEL;
        else if(strgps=="UNICORE"||strgps=="UB370"||strgps=="UB380") gpsboard=GPS_UNICORECOMM;
        else if(strgps=="HEMISPHERE"||strgps=="P307"||strgps=="UN138"||strgps=="P328") gpsboard=GPS_HEMISPHERE;
        else gpsboard=GPS_TRIMBLE;
    //}
    if(gpsboard==GPS_NOVATEL)
    {
        oem=new cNovatelCommand();
        xmlconfig->setValue("DEVICE.INFO.GPSBOARD","NOVATEL");
        xmlconfig->setValue("PORTS.GPSBOARD","NOVATEL");
		oem->setOemVersion(strgps.find("OEM7")!=string::npos?1:0);
    }
    else if(gpsboard==GPS_UNICORECOMM)
    {
        oem=new cUnicoreCommand();
        xmlconfig->setValue("DEVICE.INFO.GPSBOARD","UNICORE");
        xmlconfig->setValue("PORTS.GPSBOARD","UNICORE");
    }
    else if(gpsboard==GPS_HEMISPHERE)
    {
        oem=new cHemisphereCommand();
        xmlconfig->setValue("DEVICE.INFO.GPSBOARD","HEMISPHERE");
        xmlconfig->setValue("PORTS.GPSBOARD","HEMISPHERE");
        if(strgps=="UN138")
            xmlconfig->setValue("GPS.INFO.MODEL","UN138");
        else if(strgps=="P328")
            xmlconfig->setValue("GPS.INFO.MODEL","P328");
        else
            xmlconfig->setValue("GPS.INFO.MODEL","P307");
    }
    else
    {
        //cTrimbleCommand bd970_oem;
        //oem=&bd970_oem;
        gpsboard=GPS_TRIMBLE;
        oem=new cTrimbleCommand();
        xmlconfig->setValue("DEVICE.INFO.GPSBOARD","BD970");
        xmlconfig->setValue("PORTS.GPSBOARD","BD970");
        xmlconfig->setValue("GPS.INFO.MODEL","BD970");
    }

#else
    oem=new cTrimbleCommand();
#endif
    char tmpStr[256];
    string value;
    int n;
    n=4;//max connection

    sprintf(tmpStr,"%d",n);
    value.assign(tmpStr);
    xmlconfig->setValue("NTRIP.MAX_CONNECTION",value);
	xmlconfig->setValue("NTRIP.CUR_CONNECTION","0");

    for(i=0;i<n;i++)
    {
        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.STATUS",i);
        value.assign(tmpStr);
        xmlconfig->addNode(value);
        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.STATUS",i);
        value.assign(tmpStr);
        xmlconfig->setValue(value,"idle");

        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.STARTTIME",i);
        value.assign(tmpStr);
        xmlconfig->addNode(value);
        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.STARTTIME",i);
        value.assign(tmpStr);
        xmlconfig->setValue(value," ");

        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.TRAFFIC",i);
        value.assign(tmpStr);
        xmlconfig->addNode(value);
        sprintf(tmpStr,"DATAFLOW.NTRIP.CONNECTION@ID:%d.TRAFFIC",i);
        value.assign(tmpStr);
        xmlconfig->setValue(value,"0");
    }

    xmlconfig->getList("RECORD.SESSION@NAME",&value);
    std::vector<std::string> x = split(value.c_str(), '|');
    for(i=0;i<(int)x.size();i++)
    {
        sprintf(tmpStr,"DATAFLOW.RECORD.SESSION@NAME:%s.STATUS",x.at(i).c_str());
        value.assign(tmpStr);
        xmlconfig->addNode(value);
        xmlconfig->setValue(value,"idle");
    }

    xmlconfig->setValue("DATAFLOW.RECORD.STATUS","idle");
    xmlconfig->setValue("DATAFLOW.RECORD.SIZE","0");
	xmlconfig->setValue("SYSTEM.STATUS","idle");
	xmlconfig->setValue("SYSTEM.RUNTIME","0");
	xmlconfig->setValue("DEVICE.ENDPOINT","5");

	xmlconfig->setValue("NETWORK.GPRS.STATUS","0440");

//直连板卡/UHF设置不保存
	xmlconfig->getValue("PORTS.COM1.ENABLE",&value);
	if(value=="YES"||value=="ON")
	{
		xmlconfig->getValue("PORTS.COM1.FUNCTION",&value);
		if(value=="GPS"||value=="UHF")
		{
			xmlconfig->setValue("PORTS.COM1.FUNCTION","NMEA");
		}
	}

	if (xmlconfig->getValue("NETWORK.PRIORITY",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.PRIORITY","WAN");

    //config
    if (xmlconfig->getValue("NETWORK.PRIORITY",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.PRIORITY","WAN");

	if (xmlconfig->getValue("SYSTEM.GPS.LBAND",&value)!= RET_OK)
        xmlconfig->setValue("SYSTEM.GPS.LBAND","DISABLE");

    //gprs
    if (xmlconfig->getValue("NETWORK.GPRS.SIGNAL",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.GPRS.SIGNAL","0%");
    if (xmlconfig->getValue("NETWORK.GPRS.IMEI",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.GPRS.IMEI","0");
    if (xmlconfig->getValue("NETWORK.GPRS.SP",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.GPRS.SP","0");

    //wifi
    if (xmlconfig->getValue("NETWORK.WIFI.MCONNECTED",&value)!= RET_OK)    //connected
        xmlconfig->setValue("NETWORK.WIFI.MCONNECTED","offline");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHMODE",&value)!= RET_OK)  //server or client
        xmlconfig->setValue("NETWORK.WIFI.MESHMODE","NONE");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHDHCP",&value)!= RET_OK)  //mdhcp
        xmlconfig->setValue("NETWORK.WIFI.MESHDHCP","NONE");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHNAME",&value)!= RET_OK)  //M-SSID
        xmlconfig->setValue("NETWORK.WIFI.MESHNAME","mesh-network");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHCHANNEL",&value)!= RET_OK)   //channel
        xmlconfig->setValue("NETWORK.WIFI.MESHCHANNEL","1");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHIP",&value)!= RET_OK)        //IP
        xmlconfig->setValue("NETWORK.WIFI.MESHIP","192.168.2.1");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHMASK",&value)!= RET_OK)      //mask
        xmlconfig->setValue("NETWORK.WIFI.MESHMASK","255.255.255.0");
    if (xmlconfig->getValue("NETWORK.WIFI.MESHGATEWAY",&value)!= RET_OK)   //gateway
        xmlconfig->setValue("NETWORK.WIFI.MESHGATEWAY","192.168.2.1");



    if (xmlconfig->getValue("NETWORK.WIFI.WIFI_MODE",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.WIFI_MODE","NONE");
    if (xmlconfig->getValue("NETWORK.WIFI.AP_SSID",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.AP_SSID","NSC200");
    if (xmlconfig->getValue("NETWORK.WIFI.AP_PASS",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.AP_PASS","NONE");
    if (xmlconfig->getValue("NETWORK.WIFI.AP_IP",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.AP_IP","192.168.10.1");
    if (xmlconfig->getValue("NETWORK.WIFI.SIGNAL",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.SIGNAL","0");
    if (xmlconfig->getValue("NETWORK.WIFI.RATE",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.RATE","0");
    if (xmlconfig->getValue("NETWORK.WIFI.CHANNEL",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.CHANNEL","0");
    if (xmlconfig->getValue("NETWORK.WIFI.VIRTUALAP",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.WIFI.VIRTUALAP","OFF");

    if (xmlconfig->getValue("NETWORK.DNS1.SELECT",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.DNS1.SELECT","114.114.114.114,8.8.8.8");
    if (xmlconfig->getValue("NETWORK.DNS1.CUSTOM",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.DNS1.CUSTOM","114.114.114.114,8.8.4.4");

    //REFERENCEHEIGHT
    if (xmlconfig->getValue("SYSTEM.POSITIONING.REFERENCEHEIGHT",&value)!= RET_OK)
        xmlconfig->setValue("SYSTEM.POSITIONING.REFERENCEHEIGHT","0");

    //port
    if (xmlconfig->getValue("PORTS.SOCKET.RAW_EPH",&value)!= RET_OK)
        xmlconfig->setValue("PORTS.SOCKET.RAW_EPH","-1");
    if (xmlconfig->getValue("PORTS.SOCKET1.RAW_EPH",&value)!= RET_OK)
        xmlconfig->setValue("PORTS.SOCKET1.RAW_EPH","-1");
    if (xmlconfig->getValue("PORTS.SOCKET2.RAW_EPH",&value)!= RET_OK)
        xmlconfig->setValue("PORTS.SOCKET2.RAW_EPH","-1");

    //manager
    if (xmlconfig->getValue("SYSTEM.TIMERESTART.ENABLE",&value)!= RET_OK)
        xmlconfig->setValue("SYSTEM.TIMERESTART.ENABLE","NO");
    if (xmlconfig->getValue("SYSTEM.TIMERESTART.HOUR",&value)!= RET_OK)
        xmlconfig->setValue("SYSTEM.TIMERESTART.HOUR","00");
    if (xmlconfig->getValue("SYSTEM.TIMERESTART.MINUTE",&value)!= RET_OK)
        xmlconfig->setValue("SYSTEM.TIMERESTART.MINUTE","00");

     //DNAT
    if (xmlconfig->getValue("NETWORK.DNAT.ENABLE",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.DNAT.ENABLE","NO");

    if (xmlconfig->getValue("NETWORK.DNAT.TOTEL_LIST",&value)!= RET_OK)
    {
        xmlconfig->setValue("NETWORK.DNAT.TOTEL_LIST", "1");
        n=1;
    }
    else
    {
        n = atoi(value.c_str());
    }

    for(i=0; i<n; i++)
    {
        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.ENABLE",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"NO");
        }
        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_IP",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"0.0.0.0");
        }
        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_PORT",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"9090");
        }

        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.SOURCE_NETDEV",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"eth0");
        }

        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.DEST_IP",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"0.0.0.0");
        }
        sprintf(tmpStr,"NETWORK.DNAT.DNAT_LIST@ID:%d.DEST_PORT",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"9091");
        }
    }



	//SNAT
    if (xmlconfig->getValue("NETWORK.SNAT.ENABLE",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.SNAT.ENABLE","NO");
    if (xmlconfig->getValue("NETWORK.SNAT.TOTEL_LIST",&value)!= RET_OK)
        xmlconfig->setValue("NETWORK.SNAT.TOTEL_LIST","0");


    //n = atoi(value.c_str());
    n = 2;
    for(i=0; i<n; i++)
    {
        sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.ENABLE",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"NO");
        }

        sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.SNAT_IP",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"192.168.10.0/24");
        }

        sprintf(tmpStr,"NETWORK.SNAT.SNAT_LIST@ID:%d.DEST_DEV",i);
        if (xmlconfig->getValue(tmpStr,&value)!= RET_OK)
        {
            xmlconfig->addNode(tmpStr);
            xmlconfig->setValue(tmpStr,"wlan0");
        }
    }

#if (defined GW)
	 xmlconfig->setValue("DEVICE.INFO.MODEL","GDT-M200");
#endif

    init_recsession();
    init_endpoint();
    xmlconfig->saveConfig();
    xmlconfig->saveRealtime();
}

command::~command()
{
	/*if (m_IPCRadioCmdRead.iIPCMId!=INVALID_IPC_ID)
	{
		if(FreeSHM(&m_IPCRadioCmdRead)!=0)
		{
			printf("interface free m_IPCRadioCmdRead failed\n");
		}

	}
	if (m_IPCRadioCmdWrite.iIPCMId!=INVALID_IPC_ID)
	{

		if(FreeSHM(&m_IPCRadioCmdWrite))
		{
			printf("interface free m_IPCRadioCmdWrite failed\n");
		}

	}*/
	std::map<int, IPCMemoryINFO *>::iterator it;
	for(it=m_mapIPCCmd.begin();it!=m_mapIPCCmd.end();++it)
	{
		IPCMemoryINFO *pIPCInfo = it->second;
		if (pIPCInfo->iIPCMId!=INVALID_IPC_ID)
		{
			if(FreeSHM(pIPCInfo))
			{
				printf("interface free  m_mapIPCCmd ID %d failed\n",pIPCInfo->iIPCMId);
			}

		}
	}

	if (oem)
	{
		delete oem;
		oem=NULL;
	}
}

static TIMEZONE_TABLE timezone_table[]=
{
	{"Etc/GMT+12",-12},
	{"Etc/GMT+11",-11},
	{"Etc/GMT+10",-10},
	{"Etc/GMT+9",-9},
	{"Etc/GMT+8",-8},
	{"Etc/GMT+7",-7},
	{"Etc/GMT+6",-6},
	{"Etc/GMT+5",-5},
	{"Etc/GMT+4",-4},
	{"Etc/GMT+3",-3},
	{"Etc/GMT+2",-2},
	{"Etc/GMT+1",-1},
	{"Etc/GMT",0},
	{"Etc/GMT-1",1},
	{"Etc/GMT-2",2},
	{"Etc/GMT-3",3},
	{"Etc/GMT-4",4},
	{"Etc/GMT-5",5},
	{"Etc/GMT-6",6},
	{"Etc/GMT-7",7},
	{"Etc/GMT-8",8},
	{"Etc/GMT-9",9},
	{"Etc/GMT-10",10},
	{"Etc/GMT-11",11},
	{"Etc/GMT-12",12},
	{"Etc/GMT-13",13},
	{"Etc/GMT-14",14},
};

static  ENDPOINT_OPT endpoint_bd970[]=
{
    //{(char *)"ttyO2",PORT_SERIAL,2,0}, //COM2
    {"C192.167.100.190:5017",PORT_SOCKET,20,5017,0},
    {"C192.167.100.190:5018",PORT_SOCKET,21,5018,0},
    {"C192.167.100.190:28001",PORT_SOCKET,22,28001,0},
    {"C192.167.100.190:28002",PORT_SOCKET,23,28002,0},
    {"C192.167.100.190:5025",PORT_SOCKET,24,5025,0},
    {"C192.167.100.190:5024",PORT_SOCKET,25,5024,0},
    {"C192.167.100.190:5023",PORT_SOCKET,26,5023,0},
    {"C192.167.100.190:5022",PORT_SOCKET,27,5022,0},
    {"C192.167.100.190:5021",PORT_SOCKET,28,5021,0},
    {"C192.167.100.190:5020",PORT_SOCKET,29,5020,0},
};

static  ENDPOINT_OPT endpoint_novatel[]=
{
    //{(char *)"ttyO2",PORT_SERIAL,2,0}, //COM2
    //{"ttyUSB/novatel0",PORT_SERIAL,21,115200,0}, //USB1
    //{"ttyUSB/novatel1",PORT_SERIAL,22,115200,0}, //USB2
   // {"ttyUSB/novatel2",PORT_SERIAL,23,115200,0}, //USB1
    {"C192.167.100.190:3001",PORT_SOCKET,11,3001,0}, //ICOM1
    {"C192.167.100.190:3002",PORT_SOCKET,12,3002,0}, //ICOM2
    {"C192.167.100.190:3003",PORT_SOCKET,13,3003,0}, //ICOM3
	{"C192.167.100.190:3004",PORT_SOCKET,14,3004,0}, //ICOM4 OEM7 FW 7.03.00(June 30,2017)
	{"C192.167.100.190:3005",PORT_SOCKET,15,3005,0}, //ICOM5
	{"C192.167.100.190:3006",PORT_SOCKET,16,3006,0}, //ICOM6
	{"C192.167.100.190:3007",PORT_SOCKET,17,3007,0}, //ICOM7
};

static  ENDPOINT_OPT endpoint_unicore[]=
{
    {"C192.167.100.190:3001",PORT_SOCKET,11,3001,0}, //ICOM1
    {"C192.167.100.190:3002",PORT_SOCKET,12,3002,0}, //ICOM2
    {"C192.167.100.190:3003",PORT_SOCKET,13,3003,0}, //ICOM3
	{"C192.167.100.190:40000",PORT_ICOM,14,40000,0}, //ICOM4
	{"C192.167.100.190:40000",PORT_ICOM,15,40000,0}, //ICOM5
	{"C192.167.100.190:40000",PORT_ICOM,16,40000,0}, //ICOM6
	{"C192.167.100.190:40000",PORT_ICOM,17,40000,0}, //ICOM7
	{"C192.167.100.190:40000",PORT_ICOM,18,40000,0}, //ICOM8
	{"C192.167.100.190:40000",PORT_ICOM,19,40000,0}, //ICOM9
	{"C192.167.100.190:40000",PORT_ICOM,20,40000,0}, //ICOM10
	{"C192.167.100.190:40000",PORT_ICOM,21,40000,0}, //ICOM11
	{"C192.167.100.190:40000",PORT_ICOM,22,40000,0}, //ICOM12
	{"C192.167.100.190:40000",PORT_ICOM,23,40000,0}, //ICOM13
};

static  ENDPOINT_OPT endpoint_hemisphere[]=
{
    {(char *)"ttyUSB/cp210x0",PORT_SERIAL,2,115200,0}, //PORTB
};

static  ENDPOINT_OPT _endpoint_hemisphere[][10]=
{
	{
		{(char *)"ttyUSB/cp210x0",PORT_SERIAL,2,115200,0}, //PORTB  V2
	},
	{
		{(char *)"ttyUSB/cp210x0",PORT_SERIAL,1,115200,0}, //PORTA   V4
		{(char *)"ttyO3",PORT_SERIAL,3,115200,0}, //
	},
	{
		{(char *)"ttyO3",PORT_SERIAL,3,115200,0}, //  V4+UHF
	},
};

static RECORDSESSION_OPT record_session[]=
{
    {"",2440,0,0},
    {"",2441,1,0},
    {"",2442,2,0},
    {"",2443,3,0},
    {"",2444,4,0},
    {"",2445,5,0},
    {"",2446,6,0},
    {"",2447,7,0},
};

bool command::creat_recsession(RECORDSESSION_OPT *session,char *name)
{
    int i=0,n;
    bool find=false;
    int id=-1;
//syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession, name : %s",name);
    n=sizeof(record_session)/sizeof(RECORDSESSION_OPT);
    for(i=0; i<n; i++)
    {
        if(record_session[i].option==0||strcmp(record_session[i].name,name)==0)
        {
            find=true;
            id=i;
            break;
        }
    }

    if(find && id>=0)
    {
        record_session[id].option=1;
        //syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession, id:%d 1name : %s",id,name);
        strcpy(record_session[id].name,name);
        //syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession, n2ame : %s",name);
        memcpy(session,&record_session[id],sizeof(RECORDSESSION_OPT));
    }
    else
    {
        syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession :No More Sessions [name:%s id:%d].\n",name,id);
    }

    //syslog(LOG_LOCAL7|LOG_INFO,"creat_recsession: name %s, [%d].name %s\n",session->name,id,record_session[id].name);
    count_recsession();

    return find;
}

bool command::release_recsession(RECORDSESSION_OPT *session,char *name)
{
    int i=0,n;
    bool find=false;
    int id=-1;

    n=sizeof(record_session)/sizeof(RECORDSESSION_OPT);
    for(i=0; i<n; i++)
    {
        //syslog(LOG_LOCAL7|LOG_INFO,"session_name[%d]:%s, name %s\n",i,record_session[i].name,name);

        if(strcmp(record_session[i].name,name)==0)
        {
            find=true;
            id=i;
            break;
        }

    }

    if(find)
    {
        record_session[id].option=0;
        memset(record_session[id].name,0,sizeof(record_session[id].name));
        memcpy(session,&record_session[id],sizeof(RECORDSESSION_OPT));
    }
    else
    {
        syslog(LOG_LOCAL7|LOG_INFO,"release_recsession :Not Find Sessions [name:%s id:%d].\n",name,id);
    }

    //syslog(LOG_LOCAL7|LOG_INFO,"release_recsession: name %s, [%d].option %d\n",name,id,session->option);

    count_recsession();

    return find;
}

bool command::count_recsession()
{
    int i,m=0,n=0;
    string value;

    m=n=sizeof(record_session)/sizeof(RECORDSESSION_OPT);
    //syslog(LOG_LOCAL7|LOG_INFO,"m:%d n:%d .",m,n);
    for(i=0; i<m; i++)
    {
        if(record_session[i].option>0)
        {
            n--;
        }
        //syslog(LOG_LOCAL7|LOG_INFO,"[%d],name:%s key:%d id:%d option:%d",i,record_session[i].name,record_session[i].key,record_session[i].id,record_session[i].option);
    }

    if(m == n)
    {
        //syslog(LOG_LOCAL7|LOG_INFO,"rec Session Empty .");
        recSessionEmpty=1;
    }
    else
    {
        recSessionEmpty=0;
    }
    //syslog(LOG_LOCAL7|LOG_INFO,"count_recsession:%d .",recSessionEmpty);

    return true;
}

bool command::init_recsession()
{
    char tmpStr[64];
    string value;
    int n;

    n=sizeof(record_session)/sizeof(RECORDSESSION_OPT);

    sprintf(tmpStr,"%d",n);
    value.assign(tmpStr);
    xmlconfig->setValue("RECORD.MAX_SESSION",value);
    xmlconfig->saveConfig();

    return true;
}

bool command::init_endpoint()
{
    char tmpStr[64];
    string value;
    int n,extra=0;
    if(gpsboard==GPS_NOVATEL)
    {
		if(!oem->oem7)extra=4;
        n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
    }
    else if(gpsboard==GPS_TRIMBLE)
    {
        n=sizeof(endpoint_bd970)/sizeof(ENDPOINT_OPT);
    }
    else if(gpsboard==GPS_UNICORECOMM)
    {
        n=sizeof(endpoint_unicore)/sizeof(ENDPOINT_OPT);
    }
    else if(gpsboard==GPS_HEMISPHERE)
    {
        //output record data use raw port
        use_raw_port=1;
        xmlconfig->setValue("NTRIP.MAX_CONNECTION","3");
        string func;
        xmlconfig->getValue("PORTS.COM1.FUNCTION",&func);
        if(func=="GPS")  xmlconfig->setValue("PORTS.COM1.ENABLE","NO");

        n=sizeof(endpoint_hemisphere)/sizeof(ENDPOINT_OPT);
    }
    else
    {
        return false;
    }
    sprintf(tmpStr,"%d",n);
    value.assign(tmpStr);
    xmlconfig->setValue("DEVICE.ENDPOINT",value);
    xmlconfig->setValue("DEVICE.FREE_ENDPOINT",value);
    xmlconfig->saveConfig();
    return true;
}

bool command::count_endpoint()
{
	int i,n=0,extra=0;
	char tmpStr[64];
	string value;
	int total=0;
	ENDPOINT_OPT * p;
	if(gpsboard==GPS_NOVATEL)
	{
		if(!oem->oem7)extra=4;
		total=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
		p=endpoint_novatel;
	}
	else if(gpsboard==GPS_TRIMBLE)
	{
		total=sizeof(endpoint_bd970)/sizeof(ENDPOINT_OPT);
		p=endpoint_bd970;
	}
	else if(gpsboard==GPS_UNICORECOMM)
	{
		total=sizeof(endpoint_unicore)/sizeof(ENDPOINT_OPT);
		p=endpoint_unicore;
	}
	else if(gpsboard==GPS_HEMISPHERE)
	{
		total=sizeof(endpoint_hemisphere)/sizeof(ENDPOINT_OPT);
		p=endpoint_hemisphere;
	}
	else
		return false;

	n=total;
	for(i=0;i<total;i++)
	{
		if(p[i].pid>0)
		{
			n--;
		}
	}


	sprintf(tmpStr,"%d",n);
	value.assign(tmpStr);
	xmlconfig->setValue("DEVICE.FREE_ENDPOINT",value);
	xmlconfig->saveConfig();
	//printf("interface ---left ep num %d sizeof array %d ,ele %d\n",n,sizeof(endpoint_novatel),sizeof(ENDPOINT_OPT));
	return true;
}

bool command::creat_endpoint_nov(ENDPOINT_OPT *endpoint,int only_socket)
{
    int i,n=0,extra=0;
    bool find=false;
    int id=-1;
    int bak_id=-1;
    if(gpsboard==GPS_NOVATEL)
    {
		if(!oem->oem7)extra=4;
         n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
         for(i=0;i<n;i++)
         {
                if(endpoint_novatel[i].pid==0)
                {
                    bak_id=i;
                    if(only_socket>0)
                    {
                        if(endpoint_novatel[i].type==only_socket)
                        {
                            find=true;
                            id=i;
                            break;
                        }
                    }
                    else
                    {
                        find=true;
                        id=i;
                        break;
                    }
                }
          }
    }

    if(only_socket>0 && find==false)
    {
        if(bak_id>=0)
        {
            id=bak_id;
            find=true;
        }
    }

    if(find && id>=0)
    {
        if(gpsboard==GPS_NOVATEL)
            memcpy(endpoint,&endpoint_novatel[id],sizeof(ENDPOINT_OPT));
    }
    else
    {
        syslog(LOG_LOCAL7|LOG_INFO,"creat_endpoint :No More Ports [find:%d id:%d].\n",find,id);
    }

    syslog(LOG_LOCAL7|LOG_INFO,"creat_endpoint: id %d, pid %d\n",endpoint->id,endpoint->pid);

    return true;
}

bool command::change_endpoint(ENDPOINT_OPT *endpoint,int index)
{

    int i,n,extra=0;
    int id=0;
    bool find=false;
    if(gpsboard==GPS_NOVATEL)
    {
		if(!oem->oem7)extra=4;
        n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
         for(i=0;i<n;i++)
         {
                syslog(LOG_LOCAL7|LOG_INFO,"endpoint->id %d, struct_id %d\n",endpoint->id,endpoint_novatel[i].id);
                if(endpoint_novatel[i].id==endpoint->id)
                {
                        find=true;
                        id=i;
                        break;
                }
          }
    }
    else
    {
        return false;
    }

    if(find)
    {
        if(process[index]>0)
        {
             if(gpsboard==GPS_NOVATEL)
                endpoint_novatel[id].pid=process[index];
        }

    }


    count_endpoint();
    syslog(LOG_LOCAL7|LOG_INFO,"endpoint_novatel[%d].pid %d, process[%d] %d\n",id,endpoint_novatel[id].pid,index,process[index]);

    return true;
}

//readIPCInfo:the GNSS board commmands buffer to send
//writeIPCInfo:the buffer read from gnss board
bool command::creat_endpoint(ENDPOINT_OPT *endpoint, int index0,
                             int only_socket, const char * readIPCInfo,const char * writeIPCInfo)
{
    int i,n=0,extra=0;
    bool find=false;
    int id=-1;
    int bak_id=-1;
    if(gpsboard==GPS_NOVATEL)
    {
		if(!oem->oem7)extra=4;
		n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
		for(i=0;i<n;i++)
		{
			if(endpoint_novatel[i].pid==0)
			{
				bak_id=i;
				if(only_socket>0)
				{
					if(endpoint_novatel[i].type==only_socket)
					{
						find=true;
						id=i;
						break;
					}
				}
				else
				{
					find=true;
					id=i;
					break;
				}
			}
		}
    }
    else if(gpsboard==GPS_TRIMBLE)
    {
        n=sizeof(endpoint_bd970)/sizeof(ENDPOINT_OPT);
        for(i=0;i<n;i++)
         {
                if(endpoint_bd970[i].pid==0)
                {
                    bak_id=i;
                    if(only_socket>0)
                    {
                        if(endpoint_bd970[i].type==only_socket)
                        {
                            find=true;
                            id=i;
                            break;
                        }
                    }
                    else
                    {
                        find=true;
                        id=i;
                        break;
                    }
                }
          }
    }
    else if(gpsboard==GPS_UNICORECOMM)
    {
        n=sizeof(endpoint_unicore)/sizeof(ENDPOINT_OPT);
		for(i=0;i<n;i++)
		{
			//40000端口已打开不释放
			if(endpoint_unicore[i].pid>0 && process[index0] == endpoint_unicore[i].pid)
			{
				//syslog(LOG_LOCAL7|LOG_INFO,"creat_endpoint %d:%d exist.",endpoint_unicore[i].id,endpoint_unicore[i].pid);
				memcpy(endpoint,&endpoint_unicore[i],sizeof(ENDPOINT_OPT));
				count_endpoint();
				return true;
			}
		}
        for(i=0;i<n;i++)
        {
            if(endpoint_unicore[i].pid==0)
            {
                bak_id=i;
                if(only_socket>0)
                {
                    if(endpoint_unicore[i].type==only_socket)
                    {
                        find=true;
                        id=i;
                        break;
                    }
                }
                else
                {
                    find=true;
                    id=i;
                    break;
                }
            }
          }
    }
    else if(gpsboard==GPS_HEMISPHERE)
    {
        n=sizeof(endpoint_hemisphere)/sizeof(ENDPOINT_OPT);
        for(i=0;i<n;i++)
         {
                if(endpoint_hemisphere[i].pid==0)
                {
                    bak_id=i;
                    if(only_socket>0)
                    {
                        if(endpoint_hemisphere[i].type==only_socket)
                        {
                            find=true;
                            id=i;
                            break;
                        }
                    }
                    else
                    {
                        find=true;
                        id=i;
                        break;
                    }
                }
          }
    }
    else
    {
        return false;
    }

    if(only_socket>0 && find==false)
    {
        if(bak_id>=0)
        {
            id=bak_id;
            find=true;
        }
    }

	if(find && id>=0)
	{
		if(gpsboard==GPS_NOVATEL)
			memcpy(endpoint,&endpoint_novatel[id],sizeof(ENDPOINT_OPT));
		else if(gpsboard==GPS_TRIMBLE)
		{
			memcpy(endpoint,&endpoint_bd970[id],sizeof(ENDPOINT_OPT));
			char tmp[256];
			//cgi-bin/io.xml?port=26&type=TCP%2FIP&LocalPort=5023&Add=1
			sprintf(tmp,"cgi-bin/io.xml?port=%d&type=TCP%%2FIP&LocalPort=%d&OutputOnly=off&Add=1",endpoint->id,endpoint->option);
			oem->txtcommand(tmp);
			sprintf(tmp,"cgi-bin/io.xml?port=%d&type=TCP%%2FIP&LocalPort=%d&OutputOnly=off",endpoint->id,endpoint->option);
			oem->txtcommand(tmp);
			usleep(1000);
		}
		else if(gpsboard==GPS_UNICORECOMM)
		{
			memcpy(endpoint,&endpoint_unicore[id],sizeof(ENDPOINT_OPT));
		}
		else if(gpsboard==GPS_HEMISPHERE)
		{
			memcpy(endpoint,&endpoint_hemisphere[id],sizeof(ENDPOINT_OPT));
		}
        char* exec_argv[8] = {0};

        unsigned int len;
        if(endpoint->type==PORT_SOCKET||endpoint->type==PORT_ICOM)
        {
			//read from socket (endpoint->target,gnss board),write to buffer writeIPCInfo
			//read from buffer readIPCInfo, send to socket (endpoint->target,gnss board)
			exec_argv[0] = BuildInfoFromString(SOCKET_BIN_PATH);
			exec_argv[1] = BuildInfoFromString(endpoint->target);
			exec_argv[2] = BuildInfoFromString(readIPCInfo);
			exec_argv[3] = BuildInfoFromString(writeIPCInfo);
			start_process(4,exec_argv,process[index0]);
        }
        else//serial
        {
			//read from serial (endpoint->target,gnss board),write to buffer writeIPCInfo
			//read from buffer readIPCInfo, send to serial (endpoint->target,gnss board)
			StartProcess_Serial2(endpoint->target,"115200",readIPCInfo,writeIPCInfo,process[index0]);
        }

        if(process[index0]>0)
        {
             if(gpsboard==GPS_NOVATEL)
                endpoint_novatel[id].pid=process[index0];
            else  if(gpsboard==GPS_TRIMBLE)
                endpoint_bd970[id].pid=process[index0];
			else  if(gpsboard==GPS_UNICORECOMM){
                endpoint_unicore[id].pid=process[index0];
			}
            else  if(gpsboard==GPS_HEMISPHERE)
                endpoint_hemisphere[id].pid=process[index0];
            endpoint->pid=process[index0];

			syslog(LOG_LOCAL7|LOG_INFO,"creat_endpoint: ep source:%s id %d, pid %d\n",endpoint->target,endpoint->id,endpoint->pid);
        }
        else
        {
            syslog(LOG_LOCAL7|LOG_ERR,"creat_endpoint() error");
        }
    }else{
        syslog(LOG_LOCAL7|LOG_INFO,"creat_endpoint :No More Ports [find:%d id:%d].\n",find,id);
    }

    count_endpoint();

    return find;
}

bool command::release_endpoint(ENDPOINT_OPT *endpoint, int index)
{
	int i,n,extra=0;
	int id=0;
	bool find=false;
	if(gpsboard==GPS_NOVATEL)
	{
		if(!oem->oem7)extra=4;
		n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
		for(i=0;i<n;i++)
		{
			//syslog(LOG_LOCAL7|LOG_INFO,"id %d, pid %d\n",process[index],endpoint_novatel[i].pid);
			if(endpoint_novatel[i].pid==process[index])
			{
				find=true;
				id=i;
				break;
			}
		}
	}
	else if(gpsboard==GPS_TRIMBLE)
	{
		n=sizeof(endpoint_bd970)/sizeof(ENDPOINT_OPT);
		for(i=0;i<n;i++)
		{
			if(endpoint_bd970[i].pid==process[index])
			{
				find=true;
				id=i;
				break;
			}
		}
	}
	else if(gpsboard==GPS_UNICORECOMM)
	{
		n=sizeof(endpoint_unicore)/sizeof(ENDPOINT_OPT);
		for(i=0;i<n;i++)
		{
			if(endpoint_unicore[i].pid==process[index])
			{
				find=true;
				id=i;
				if(endpoint_unicore[id].type == PORT_ICOM)
				{//syslog(LOG_LOCAL7|LOG_INFO,"pid %d, pid %d\n",process[index],endpoint_unicore[i].pid);
					return false;
				}
				break;
			}
		}
	}
	else if(gpsboard==GPS_HEMISPHERE)
	{
		n=sizeof(endpoint_hemisphere)/sizeof(ENDPOINT_OPT);
		for(i=0;i<n;i++)
		{
			if(endpoint_hemisphere[i].pid==process[index])
			{
				find=true;
				id=i;
				break;
			}
		}
	}
	else
	{
		return false;
	}

	if(find)
	{
		//syslog(LOG_LOCAL7|LOG_INFO,"found id %d",id);
		if(gpsboard==GPS_NOVATEL)
		{
			memcpy(endpoint,&endpoint_novatel[id],sizeof(ENDPOINT_OPT));
			endpoint_novatel[id].pid=0;
		}
		else if(gpsboard==GPS_TRIMBLE)
		{
			memcpy(endpoint,&endpoint_bd970[id],sizeof(ENDPOINT_OPT));
			endpoint_bd970[id].pid=0;
		}
		else if(gpsboard==GPS_UNICORECOMM)
		{
			memcpy(endpoint,&endpoint_unicore[id],sizeof(ENDPOINT_OPT));
			endpoint_unicore[id].pid=0;
		}
		else if(gpsboard==GPS_HEMISPHERE)
		{
			memcpy(endpoint,&endpoint_hemisphere[id],sizeof(ENDPOINT_OPT));
			endpoint_hemisphere[id].pid=0;
		}
		syslog(LOG_LOCAL7|LOG_INFO,"release_endpoint: id %d, pid %d\n",endpoint->id,endpoint->pid);
	}
	if(process[index]>0) stop_process(process[index]);
	count_endpoint();
	return find;
}


bool command::find_endpoint(ENDPOINT_OPT *endpoint, int index)
{
    int i,n,extra=0;
    int id=0;
    bool find=false;
    if(gpsboard==GPS_NOVATEL)
    {
		if(!oem->oem7)extra=4;
        n=sizeof(endpoint_novatel)/sizeof(ENDPOINT_OPT)-extra;
         for(i=0;i<n;i++)
         {
                //syslog(LOG_LOCAL7|LOG_INFO,"id %d, pid %d\n",process[pid],endpoint_novatel[i].pid);
                if(endpoint_novatel[i].pid!=0 && endpoint_novatel[i].pid==process[index])
                {
                        find=true;
                        id=i;
                        break;
                }
          }
    }
    else if(gpsboard==GPS_TRIMBLE)
    {
        n=sizeof(endpoint_bd970)/sizeof(ENDPOINT_OPT);
         for(i=0;i<n;i++)
         {
                if(endpoint_bd970[i].pid!=0 && endpoint_bd970[i].pid==process[index])
                {
                        find=true;
                        id=i;
                        break;
                }
          }
    }
    else if(gpsboard==GPS_UNICORECOMM)
    {
        n=sizeof(endpoint_unicore)/sizeof(ENDPOINT_OPT);
         for(i=0;i<n;i++)
         {
                if(endpoint_unicore[i].pid!=0 && endpoint_unicore[i].pid==process[index])
                {
                        find=true;
                        id=i;
                        break;
                }
          }
    }
    else if(gpsboard==GPS_HEMISPHERE)
    {
        n=sizeof(endpoint_hemisphere)/sizeof(ENDPOINT_OPT);
         for(i=0;i<n;i++)
         {
                if(endpoint_hemisphere[i].pid!=0 && endpoint_hemisphere[i].pid==process[index])
                {
                        find=true;
                        id=i;
                        break;
                }
          }
    }
    else
    {
        return false;
    }

    if(find)
    {
            //syslog(LOG_LOCAL7|LOG_INFO,"found id %d",id);
            if(gpsboard==GPS_NOVATEL)
            {
                memcpy(endpoint,&endpoint_novatel[id],sizeof(ENDPOINT_OPT));
            }
            else if(gpsboard==GPS_TRIMBLE)
            {
                memcpy(endpoint,&endpoint_bd970[id],sizeof(ENDPOINT_OPT));
            }
            else if(gpsboard==GPS_UNICORECOMM)
            {
                memcpy(endpoint,&endpoint_unicore[id],sizeof(ENDPOINT_OPT));
            }
            else if(gpsboard==GPS_HEMISPHERE)
            {
                memcpy(endpoint,&endpoint_hemisphere[id],sizeof(ENDPOINT_OPT));
            }
        syslog(LOG_LOCAL7|LOG_INFO,"find_endpoint: id %d, pid %d\n",endpoint->id,endpoint->pid);
    }
    return find;
}

void command::set_sysTimezone(int timezone)
{
	char tmpStr[256]={0},tz[16];

	/*int i=0,n;
	bool find=false;
	int id=-1;

	n=sizeof(timezone_table)/sizeof(TIMEZONE_TABLE);
	for(i=0; i<n; i++)
	{
		if(timezone_table[i].zone==timezone)
		{
			find=true;
			id=i;
			break;
		}
	}

	if(find && id>=0)
	{
		sprintf(tmpStr,"ln -s /usr/share/zoneinfo/%s /etc/localtime",timezone_table[id].zoneinfo);

		system("rm -f /etc/localtime");
		system(tmpStr);
	}*/

	if(timezone>0)sprintf(tz,"-%d",timezone);
	else sprintf(tz,"+%d",-timezone);
	sprintf(tmpStr,"ln -s /usr/share/zoneinfo/Etc/GMT%s /etc/localtime",tz);

	system("rm -f /etc/localtime");
	system(tmpStr);

	sprintf(tmpStr,"sed -i \"s#^date.timezone =.*#date.timezone = Etc/GMT%s#g\" %s",tz,PHP_INI);
    system(tmpStr);

	return ;
}

void command::ctl_setDataLink(int mode)
{

	if(SysParam.radio)
	{
		if(mode==MODE_LINK_1)mode=MODE_LINK_6;
	}

	if(mode==MODE_LINK_1)
	{
		cur_cpld_mode=1;
		if(cur_cpld_mode!=last_cpld_mode)
		{
			if(SysParam.hardver>=4)
			{
				system("insmod /lib/modules/cpld-mode1.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode2 1> /dev/null  2>&1");
				system("rmmod cpld-mode3 1> /dev/null  2>&1");
				system("rmmod cpld-mode4 1> /dev/null  2>&1");
				system("rmmod cpld-mode5 1> /dev/null  2>&1");
			}else
			{
				default_cpld_mode();
			}
		}
	}
	else if(mode==MODE_LINK_2)
	{

	}
	else if(mode==MODE_LINK_6)
	{
		cur_cpld_mode=6;
		if(cur_cpld_mode!=last_cpld_mode)
		{
			if(SysParam.hardver>=4)
			{
				system("rmmod cpld-mode1 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode2.ko 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode3.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode4 1> /dev/null  2>&1");
				system("rmmod cpld-mode5 1> /dev/null  2>&1");
			}else
			{
				default_cpld_mode();
			}
		}
	}
	else if(mode==MODE_LINK_7)
	{

	}
	else if(mode==MODE_LINK_8)
	{
		cur_cpld_mode=8;
		if(cur_cpld_mode!=last_cpld_mode)
		{
			if(SysParam.hardver>=4)
			{
				system("rmmod cpld-mode1 1> /dev/null  2>&1");
				system("rmmod cpld-mode2 1> /dev/null  2>&1");
				system("rmmod cpld-mode3 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode4.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode5 1> /dev/null  2>&1");
			}else
			{
				default_cpld_mode();
			}
		}
	}
	else if(mode==MODE_LINK_9)
	{

	}
	else if(mode==MODE_LINK_10)
	{

	}
	else if(mode==MODE_LINK_GPS)
	{
		cur_cpld_mode=20;
		if(cur_cpld_mode!=last_cpld_mode)
		{
			if(SysParam.hardver>=4)
			{
				if(littlegprsboard)//P307
				{
					system("insmod /lib/modules/cpld-mode2.ko 1> /dev/null  2>&1");
				}
				else
				{
					system("rmmod cpld-mode2 1> /dev/null  2>&1");
				}
				system("rmmod cpld-mode1 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode3.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode4 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode5.ko 1> /dev/null  2>&1");
			}
			else
			{
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
			}
		}
	}
	else if(mode==MODE_LINK_UHF)
	{
		cur_cpld_mode=23;
		if(cur_cpld_mode!=last_cpld_mode)
		{
			if(SysParam.hardver>=4)
			{
				system("insmod /lib/modules/cpld-mode1.ko 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode2.ko 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode3.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode4 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode5.ko 1> /dev/null  2>&1");
			}
			else
			{
				system("rmmod cpld-mode1 1> /dev/null  2>&1");
				system("rmmod cpld-mode2 1> /dev/null  2>&1");
				system("rmmod cpld-mode3 1> /dev/null  2>&1");
				system("insmod /lib/modules/cpld-mode4.ko 1> /dev/null  2>&1");
				system("rmmod cpld-mode5 1> /dev/null  2>&1");
			}
			system((char *)"insmod /lib/modules/uhf-enable.ko  1> /dev/null  2>&1");
			if(SysParam.direct_uhf)
			{
				system((char *)"insmod /lib/modules/uhf-config.ko  1> /dev/null  2>&1");
				system((char *)"rmmod uhf-config  1> /dev/null  2>&1");
			}
		}
	}

	last_cpld_mode = cur_cpld_mode;
}

void command::default_cpld_mode()
{
    if(gpsboard==GPS_HEMISPHERE)
    {
        system("rmmod cpld-mode1 1> /dev/null  2>&1");
        system("rmmod cpld-mode2 1> /dev/null  2>&1");
        system("rmmod cpld-mode3 1> /dev/null  2>&1");
        system("rmmod cpld-mode4 1> /dev/null  2>&1");
		system("rmmod cpld-mode5 1> /dev/null  2>&1");
        system("insmod /lib/modules/cpld-mode3.ko 1> /dev/null  2>&1");
    }
    else
    {
#ifdef NSC200
            system("rmmod cpld-mode1 1> /dev/null  2>&1");
            system("rmmod cpld-mode2 1> /dev/null  2>&1");
            system("rmmod cpld-mode3 1> /dev/null  2>&1");
            system("rmmod cpld-mode4 1> /dev/null  2>&1");
			system("rmmod cpld-mode5 1> /dev/null  2>&1");
#else
            system("rmmod gpio158 1> /dev/null  2>&1");
            system("rmmod gpio159 1> /dev/null  2>&1");
            system("rmmod gpio160 1> /dev/null  2>&1");
#endif
    }
}


#ifdef ANTENNA_MANAGEMENT
const ANTENNA_INFO antennas[]=
{
    {
        "HXCGX606A_HXCS",
        "0.5 -0.8 122.1 0.0 0.4 0.4 0.2 -0.3 -1.0 -1.8 -2.4 -2.9 -3.1 -3.1 -2.8 -2.2 -1.5 -0.3 1.3 3.6 0.0 0.0 0.1 -1.0 145.3 0.0 -0.7 -1.2 -1.6 -1.9 -2.3 -2.6 -3.0 -3.3 -3.7 -3.8 -3.7 -3.3 -2.7 -1.9 -0.8 0.8 0.0 0.0",//igs
        "-0.1 -0.4 140.2 0.0 0.6 1.3 2.1 2.9 3.6 4.2 4.7 5.0 5.1 5.0 4.6 4.0 3.1 2.0 0.6 -0.9 0.0 0.0 0.2 -0.4 152.9 0.0 -0.6 -0.7 -0.5 -0.1 0.3 0.8 1.2 1.5 1.5 1.5 1.2 0.7 0.1 -0.6 -1.3 -2.0 0.0 0.0",//igr
        122.1,145.3
    },
    //
    {
        "HXCGX606A_NONE",
        "0.7 -1.0 122.3 0.0 0.4 0.4 0.1 -0.5 -1.3 -2.2 -2.9 -3.5 -3.7 -3.7 -3.3 -2.7 -1.8 -0.4 1.4 4.0 0.0 0.0 0.0 -1.0 143.1 0.0 -0.8 -1.5 -2.0 -2.4 -2.8 -3.1 -3.6 -4.0 -4.3 -4.4 -4.4 -3.9 -3.2 -2.3 -1.0 0.9 0.0 0.0",//igs
        "0.1 -0.6 140.5 0.0 0.6 1.3 2.0 2.7 3.3 3.8 4.2 4.4 4.5 4.4 4.1 3.5 2.8 1.9 0.7 -0.5 0.0 0.0 0.1 -0.4 150.8 0.0 -0.7 -1.0 -0.9 -0.6 -0.2 0.3 0.6 0.8 0.9 0.8 0.5 0.1 -0.4 -1.0 -1.5 -1.8 0.0 0.0",//igr
        122.3,143.1
    },
    //
    {
        "HXCCSX601A_NONE",
        "0.3 -0.8 57.7 0.0 0.8 1.2 1.4 1.2 1.0 0.5 0.2 0.0 -0.1 -0.2 -0.3 -0.4 -0.8 -1.4 -2.2 -3.2 0.0 0.0 1.8 -0.4 48.5 0.0 0.8 1.2 1.3 1.3 1.0 0.7 0.2 -0.2 -0.6 -0.8 -0.6 -0.1 0.5 1.2 2.0 3.0 0.0 0.0",//igs
        "-0.1 -0.4 140.2 0.0 0.6 1.3 2.1 2.9 3.6 4.2 4.7 5.0 5.1 5.0 4.6 4.0 3.1 2.0 0.6 -0.9 0.0 0.0 0.2 -0.4 152.9 0.0 -0.6 -0.7 -0.5 -0.1 0.3 0.8 1.2 1.5 1.5 1.5 1.2 0.7 0.1 -0.6 -1.3 -2.0 0.0 0.0",//igr
        57.7,48.5
    },
};


const char ant_none[]="0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";


int command::initAntennaModel()
{
    int i;
    int find=0;
    char cmd[768];

    if(gpsboard!=GPS_NOVATEL) return 0;

    string tmp;
    string ant_type,ant_serial;
    xmlconfig->getValue("SYSTEM.ANTENNA.TYPE",&ant_type);

    if(ant_type.length()>0)
    {
               //search in rom
                for(i=0;i<(int)(sizeof(antennas)/sizeof(ANTENNA_INFO));i++)
                {
                    if(strcmp(antennas[i].type,ant_type.c_str())==0)
                    {
                        ant_serial=ant_type;
                        if(ant_type.find("_")!=std::string::npos)
                        {
                            int p1=ant_type.find("_");
                            tmp=ant_type.substr(0,p1);
                            ant_serial=ant_type.substr(p1+1,ant_type.length()-p1-1);
                            ant_type=tmp;
                        }
                        /*if(SysParam.ant_useigr)
                        {
                            sprintf(cmd,"baseantennamodel %s %s 1 user %s\r\n",
                                    ant_type.c_str(),ant_type.c_str(), antennas[i].igr);
                        }
                        else*/
                        {
                            sprintf(cmd,"antennamodel %s %s 1 user %s\r\n",
                                    ant_type.c_str(),ant_serial.c_str(), antennas[i].igs);
                        }
                        find=1;
                        break;
                    }
                }
         }
        if(find)
        {

        }
        else
        {
            syslog(LOG_LOCAL7|LOG_INFO,"-I- Not found ant type, use none settings.");
            sprintf(cmd,"antennamodel none 0 1 user %s\r\n",ant_none);
        }

        if(!oem->txtcommand((char *)cmd))  return RET_EXEC_FAILED;
        syslog(LOG_LOCAL7|LOG_INFO,"-I- %s",(char *)cmd);
        if(!oem->txtcommand((char *)"rtkantenna arp enable\r\n"))  return RET_EXEC_FAILED;
        syslog(LOG_LOCAL7|LOG_INFO,"-I- rtkantenna arp enable\r\n");
        return 0;
}


int command::setBaseAntennaModel()
{
    int i;
    int find=0;
    char cmd[768];

    if(gpsboard!=GPS_NOVATEL) return 0;

    string tmp;
    string ant_type,ant_serial;
    xmlconfig->getValue("SYSTEM.ANTENNA.TYPE",&ant_type);

    if(ant_type.length()>0)
    {
               //search in rom
                for(i=0;i<(int)(sizeof(antennas)/sizeof(ANTENNA_INFO));i++)
                {
                    if(strcmp(antennas[i].type,ant_type.c_str())==0)
                    {
                        ant_serial=ant_type;
                        if(ant_type.find("_")!=std::string::npos)
                        {
                            int p1=ant_type.find("_");
                            tmp=ant_type.substr(0,p1);
                            ant_serial=ant_type.substr(p1+1,ant_type.length()-p1-1);
                            ant_type=tmp;
                        }
                        /*if(SysParam.ant_useigr)
                        {
                            sprintf(cmd,"baseantennamodel %s %s 1 user %s\r\n",
                                    ant_type.c_str(),ant_type.c_str(), antennas[i].igr);
                        }
                        else*/
                        {
                            sprintf(cmd,"baseantennamodel %s %s 1 user %s\r\n",
                                    ant_type.c_str(),ant_serial.c_str(), antennas[i].igs);
                        }
                        find=1;
                        break;
                    }
                }
         }
        if(find)
        {

        }
        else
        {
            syslog(LOG_LOCAL7|LOG_INFO,"-I- Not found ant type, use none settings.");
            sprintf(cmd,"baseantennamodel none 0 1 user %s\r\n",ant_none);
        }

        if(!oem->txtcommand((char *)cmd))  return RET_EXEC_FAILED;
        syslog(LOG_LOCAL7|LOG_INFO,"-I- %s",(char *)cmd);
        return 0;
}

#endif

//长时间阻塞的命令需要清空消息队列，避免满后出错
void command::clear_msg(unsigned int type)//PROC_INTERFACE
{
    MSG_BUFF msg;
    int qid;
    if ((qid = msgget(KEY_MSGQUEUE,0666)) == -1)
    {
        //syslog(LOG_LOCAL7|LOG_ERR,"creat message queue error");
        return;
    }
    while (msgrcv(qid, &msg, MAX_MSG_SIZE,type , IPC_NOWAIT)>=0);//clear
}


void command::ShowMsg(const string str)
{
	returnMsg += str ;
	//returnMsg += "\n";
}


string command::AssemblyMsgLine(const string str)
{
    char tmp[32];
    string line;
    if(str.length()>0)
    {
        line="@GNSS,"+str;
        //cout<<line<<endl;
        sprintf(tmp,"*%02X\r\n",cmd_crc8((unsigned char   *)line.c_str(),line.length()));
        line.append(tmp) ;
    }
	return line;
}

void command::ShowMsgLine(const string str)
{
    char tmp[32];
    string line;
    if(str.length()>0)
    {
        line="@GNSS,"+str+"";
        //cout<<line<<endl;
        sprintf(tmp,"*%02X\r\n",cmd_crc8((unsigned char   *)line.c_str(),line.length()));
        line.append(tmp);
        returnMsg += line ;
    }
	//returnMsg += "\n";
}


TXT_CMD* command::FindCMD(string str)
{
	unsigned int i;
    string cmd=str;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	//for (i = 0; i<cmd.length(); i++)
	//	cmd[i] = toupper(cmd[i]);

    for (i = 0; i < COUNTOF(CmdTable); i++)
    {
        if (cmd.compare(CmdTable[i].cmdname) == 0  )
        {
            return &CmdTable[i];	/* full match */
        }
    }

	return NULL;	/* not found */
}

int command::ParseLine(string line, string (&argv)[MAX_CMD_ARGS])
{
	int nargs = 0;
	unsigned int index = 0;
	unsigned int start,end;
	char indexChar;
	bool inQuotation;
	//int len;
	string str;

	//len = line.length();
	//Trim the string

    indexChar = line.at(index);
    while (((indexChar == ' ') || (indexChar == '\t') || (indexChar == ',')) && (index<line.length()))
    {
        ++index;
        indexChar = line.at(index);
    }
    start = index;
    index = line.length() - 1;
    indexChar = line.at(index);
    while (((indexChar == ' ') || (indexChar <0x20) || (indexChar == ',')) && (index>start))
    {
        --index;
        indexChar = line.at(index);
    }
    end = index+1;
    line = line.substr(start,end-start);


	//ShowMsg("Trim result: "+line);

    //Split the string
    inQuotation = false;
    index = 0;
    while ((nargs < MAX_CMD_ARGS) && (index < line.length()))
    {
        start = index;
        if (line[index] == '"')
            inQuotation = !inQuotation;
        while ((index < line.length()) && ((line[index] != ',') || (inQuotation == true)))
        {
            index++;
            if (line[index] == '"')
                inQuotation = !inQuotation;
        }
        end = index;
        argv[nargs] = line.substr(start,end - start);


		//Test
		//ShowDebug("Session :" + argv[nargs]);
		nargs++;
		index++;
	}

	if (nargs >= MAX_CMD_ARGS)
	{
		ShowMsg("<ERROR: Too many args!>");
	}
	return (nargs);
}

void command::CMDParse(string strCMD,long source)
{
	int argc,result;
	string argv[MAX_CMD_ARGS];
	TXT_CMD *pCmd;
	//bool result;
	string tempStr;
	//char *tmpPtr;
	ostringstream ost;

    SetCmdSource(source);
	returnMsg.clear();
	//lrd_test
	//cout << "Received:" << strCMD << endl;
	curCommand=strCMD;
	/* Extract arguments */
	if ((argc = ParseLine(strCMD, argv)) == 0)
		return;	// no command

	/* Look up command in command table */
	if ((pCmd = FindCMD(argv[0])) != NULL)
	{
		argc--; // skip command name
		/* found - check args */
		//printf("argc: %d\n",argc);
		if (argc < pCmd->minargs || argc > pCmd->maxargs) {
		    cout << "ERROR: Parameter Not Supported! " << endl;
			//ShowMsg("<ERROR: Parameter Not Supported! Try 'HELP'.>\r\n");
			result = RET_ERR_PARAM;
		}
		else
		{
			result = (this->*(pCmd->doCmd))(argc, argv) ;
		}
	}
	else //Can not find command
	{
		//ShowMsg("<ERROR: Unknown Command . Try 'HELP'.>\r\n");
		result = RET_INVALID_CMD;
	}
	ost.clear();
	switch (result)
	{
	case RET_OK:
		//ost << argv[0] << ",OK";
		break;
	case RET_INVALID_CMD:
		ost <<strCMD<<",ERROR,UNKNOWN_COMMAND";
		break;
	case RET_INVALID_NODE:
		ost << strCMD<< ",ERROR,INVALID_NODE";
		break;
	case RET_INVALID_VALUE:
		ost << strCMD<< ",ERROR,INVALID_VALUE";
		break;
	case RET_INVALID_OPERATION:
		ost << strCMD<< ",ERROR,INVALID_OPERATION";
		break;
	case RET_ERR_PARAM:
		ost << strCMD<< ",ERROR,WRONG_PARAM";
		break;
	case RET_EXEC_FAILED:
		ost << strCMD<< ",ERROR,EXEC_FAILED";
		break;
    case RET_UNSUPPORTED_NOW:
		ost << strCMD<< ",ERROR,UNSUPPORTED_NOW";
		break;
	default:
		break;
	}
	//ost << endl;
	//ShowMsg(ost.str());
	ShowMsgLine(ost.str());
}


int command::GetEnumArgIndex(const char *enumv[], const char *argv)
{
	int i = 0;

	while (NULL != enumv[i])
	{
		if (strcmp(argv, enumv[i]) == 0)
			return (i);
		i++;
	}
	return -1;
}


int command::GetDigitFromString(const string str)
{
	int i;
	i = atoi(str.c_str());
	return i;
}

bool command::SetCmdSource(long source)
{
    cmd_source=source;
    if(cmd_source==PROC_DECODER) oem_msg_counter++;
    return true;
}

bool command::init_gps()
{
	last_cpld_mode=cur_cpld_mode=0;
	system((char *)"insmod /lib/modules/cpld-mode1.ko  1> /dev/null  2>&1") ;
	system((char *)"insmod /lib/modules/cpld-mode2.ko  1> /dev/null  2>&1") ;
	system((char *)"insmod /lib/modules/cpld-mode3.ko  1> /dev/null  2>&1") ;
	system((char *)"insmod /lib/modules/cpld-mode4.ko  1> /dev/null  2>&1") ;
	system((char *)"insmod /lib/modules/cpld-mode5.ko  1> /dev/null  2>&1") ;

	sleep(1);
	//default_cpld_mode();
	ctl_setDataLink(MODE_LINK_1);

	char* exec_argv[8] = {0};
	unsigned int n;
	int iSize8K=IPC_GPS_READ_SIZE8K;
	//lrd_test
	printf("init_gps:%d \n",gpsboard);
	//interface create process oem, oem create process serial2
	//interface send cmd to fifo IPC_GPS_RAW_WRITE, serial2 read fifo IPC_GPS_RAW_WRITE, send to GNSS board
	//serial2 get gnss response/data, write to fifo IPC_GPS_RAW_READ, oem parse data, send result to msgqueue,RECORD_IPC_INFO,IPC_NMEA_INFO
	if(gpsboard==GPS_NOVATEL)
	{
		//lrd_test
		system("/geo/app/scripts/novatel.sh &");
		exec_argv[0] = BuildInfoFromString(OEM_NOVATEL_BIN_PATH);
		exec_argv[2] = BuildIPCInfo(IPC_TCP_RECORD,IPC_RECORD_SIZE8K);
	}
	else if(gpsboard==GPS_UNICORECOMM)
	{
		exec_argv[0] = BuildInfoFromString(OEM_UNICORE_BIN_PATH);
		exec_argv[2] =  BuildIPCInfo(IPC_TCP_RECORD,IPC_RECORD_SIZE8K);
	}
	else if(gpsboard==GPS_HEMISPHERE)
	{
		//bigger fifo for hemisphere
		iSize8K=IPC_GPS_RAW_READ_SIZE8K;
		exec_argv[0] = BuildInfoFromString(OEM_HEMISPHERE_BIN_PATH);
		exec_argv[2] = BuildInfoFromString(RECORD_IPC_INFO);
	}
	else
	{
		exec_argv[0] = BuildInfoFromString(OEM_BIN_PATH);
		exec_argv[2] =  BuildIPCInfo(IPC_TCP_RECORD,IPC_RECORD_SIZE8K);
	}
	exec_argv[1] = BuildIPCInfo(IPC_GPS_RAW_READ,iSize8K);
	exec_argv[3] = BuildInfoFromString(IPC_NMEA_INFO);
	exec_argv[4] =BuildIPCInfo(IPC_GPS_CONF_WRITE,IPC_GPS_CONF_WRITE_SIZE8K);
	//MessageQueueId
	n=3;
	exec_argv[5] = (char *)malloc(n+1);
	sprintf(exec_argv[5], "%d",PROC_DECODER);

	start_process(6,exec_argv,process[PROCESS_OEM]);

	return true;
}


bool command::init_oled()
{
    char* exec_argv[8] = {0};
    unsigned int n;

    if(process[PROCESS_OLED]) stop_process(process[PROCESS_OLED]);

	system("kill `ps | grep -v grep | grep OLED | awk '{print $1}'` 1> /dev/null 2>&1");
#ifdef GW
	exec_argv[0] = BuildInfoFromString(OLED2_BIN_PATH);
#else
    exec_argv[0] = BuildInfoFromString(OLED_BIN_PATH);
#endif
    start_process(1,exec_argv,process[PROCESS_OLED]);

    return true;
}

bool command::init_oled_upgrade()
{
    char* exec_argv[8] = {0};
    unsigned int n;
    string upgrade="upgrade";

    if(process[PROCESS_OLED]) stop_process(process[PROCESS_OLED]);

    exec_argv[0] = BuildInfoFromString(OLED_BIN_PATH);
    exec_argv[1] = BuildInfoFromString(upgrade.c_str());
    start_process(2,exec_argv,process[PROCESS_OLED]);
    return true;
}

extern unsigned char authcode[64];
bool command::get_infos()
{
    string value;
    char *p;
    char str[128];
    char tmp[2048];
    bool checkauthcode;
#ifndef DEBUG
    string  serial;

    memset(tmp,0,sizeof(tmp));
    if(!eeprom_read(DEV_INFO_ADDR,(unsigned char *)tmp,1024))
    {
        syslog(LOG_LOCAL7|LOG_ERR,"read eeprom error.");
    }
    if(strstr((char *)tmp,"[DEVICE_SERIAL]")>0)
    {
        Ctl_GetMachInfoValue(tmp,"DEVICE_SERIAL",str,"SC200105010000");
        serial.assign(str);
        xmlconfig->setValue("DEVICE.INFO.SERIAL", serial);
        xmlconfig->setValue("NETWORK.WIFI.AP_SSID", serial);

		Ctl_GetMachInfoValue(tmp,"MESH",str,"OFF");
		value.assign(str);
		xmlconfig->setValue("NETWORK.WIFI.MESH", value);
		if(strstr(str,"OFF")!=NULL)
		{
			xmlconfig->getValue("NETWORK.WIFI.ENABLE",&value);
			transform(value.begin(), value.end(), value.begin(), ::toupper);
			if(value.compare("YES") == 0)
			{
				xmlconfig->getValue("NETWORK.WIFI.WIFI_MODE",&value);
				transform(value.begin(), value.end(), value.begin(), ::toupper);
				if(value.compare("MESH") == 0)
				{
					xmlconfig->setValue("NETWORK.WIFI.ENABLE","NO");
				}
			}
		}

        Ctl_GetMachInfoValue(tmp,"MODEL",str,"SC200");
        value.assign(str);
        xmlconfig->setValue("DEVICE.INFO.MODEL", value);

        if(strstr(str, "WD100")!=NULL)
        {
            checkreg=0;
            syslog(LOG_LOCAL7|LOG_INFO,"No need register\n");
        }
        else
        {
            checkreg=1;
            syslog(LOG_LOCAL7|LOG_INFO,"Need register\n");
        }

        Ctl_GetMachInfoValue(tmp,"HARDWARE_VERSION",str,"UNKNOWN");
        value.assign(str);
        xmlconfig->setValue("DEVICE.INFO.HARDWARE_VER", value);

        if(strstr(str, "RS485")!=NULL)
        {
            rs485_flag=1;
            printf("NSC200 with RS485\n");
        }
        else
        {
            rs485_flag=0;
            printf("NSC200 without RS485\n");
        }

		if(strstr(str, "NSC200-V1")!=NULL)
		{
			SysParam.hardver=1;
		}
        else if(strstr(str, "NSC200-V2")!=NULL)
        {
            SysParam.hardver=2;
        }
        else if(strstr(str, "NSC200-V3")!=NULL)
        {
            SysParam.hardver=3;
        }
		else if(strstr(str, "NSC200-V4")!=NULL)
		{
			SysParam.hardver=4;
		}
        else
        {
            SysParam.hardver=1;
        }

		if(strstr(str, "NSC200-V4.1")!=NULL)
		{
			xmlconfig->setValue("PORTS.BLUETOOTH.MODULE", "NO");
			xmlconfig->setValue("PORTS.BLUETOOTH.ENABLE", "NO");
			xmlconfig->setValue("NETWORK.WIFI.MODULE","NO");
			xmlconfig->setValue("NETWORK.WIFI.ENABLE","NO");
			xmlconfig->setValue("NETWORK.FTP.ENABLE_ANONYMOUS","NO");
		}else
		{
			xmlconfig->setValue("PORTS.BLUETOOTH.MODULE", "YES");
			xmlconfig->setValue("NETWORK.WIFI.MODULE","YES");
		}

        Ctl_GetMachInfoValue(tmp,"MANUFACTURE_DATE",str,"UNKNOWN");
        value.assign(str);
        xmlconfig->setValue("DEVICE.INFO.MANUFACTURE_DATE", value);

        Ctl_GetMachInfoValue(tmp,"GPSBOARD",str,"BD970");
        value.assign(str);
        xmlconfig->setValue("DEVICE.INFO.GPSBOARD", value);

		if((strstr(str, "UN138")!=NULL)||(strstr(str, "P328")!=NULL))
		{
			endpoint_hemisphere[0].id=3; //PORTC
		}
		if(strstr(str, "P307")!=NULL){
			littlegprsboard=1;
		}

		if(SysParam.hardver>=4)
		{
			endpoint_hemisphere[0].target="ttyO3";
		}

		xmlconfig->setValue("RADIO.FREQ_UPPER_LIMIT","470");
        xmlconfig->setValue("RADIO.FREQ_LOWER_LIMIT","410");
        SysParam.radio=0;
        SysParam.direct_uhf=0;
        Ctl_GetMachInfoValue(tmp,"DATALINK_TYPE",str,"UNKNOWN");
        if((p=strstr(str,"U1006"))>0)
        {
            xmlconfig->setValue("RADIO.INFO.MODEL","DU1006D");
            SysParam.radio=1;
            if(strstr(p,"+")>0)
            {
                SysParam.direct_uhf=1;
            }
            xmlconfig->setValue("RADIO.AVAILABLE_MODE","2|3|4|9|10");
        }
        else  if((p=strstr(str,"U1007"))>0)
        {
            xmlconfig->setValue("RADIO.INFO.MODEL","DU1007D");
            SysParam.radio=1;
            SysParam.direct_uhf=1;
            xmlconfig->setValue("RADIO.AVAILABLE_MODE","0|2|3|4|9|10");
        }
        else if(strstr(str,"TRM10")>0)
        {
            if((strstr(str,"TRM101"))>0) xmlconfig->setValue("RADIO.INFO.MODEL","TRM101");
            else xmlconfig->setValue("RADIO.INFO.MODEL","TRM100");
            SysParam.radio=1;
            SysParam.direct_uhf=1;
            xmlconfig->setValue("RADIO.AVAILABLE_MODE","0|2|3|4|9|10");
        }
        else if(strstr(str,"XDL")>0)
        {
            SysParam.radio=1;
            xmlconfig->setValue("RADIO.INFO.MODEL","XDL");
            if(strstr(str,"XDL+")>0)
            {

            }
            SysParam.direct_uhf=1;
            xmlconfig->setValue("RADIO.AVAILABLE_MODE","0|2|3|4|9|10");
            xmlconfig->setValue("RADIO.FREQ_UPPER_LIMIT","473");
            xmlconfig->setValue("RADIO.FREQ_LOWER_LIMIT","403");

            //check radio options
            xmlconfig->getValue("RADIO.MODULATION",&value);//4FSK GMSK
            if(value.length()==0)
            {
                value="GMSK";
                xmlconfig->setValue("RADIO.MODULATION",value);
            }
            xmlconfig->getValue("RADIO.LINKSPEED",&value);//4800 9600
            if(value.length()==0)
            {
                value="9600";
                xmlconfig->setValue("RADIO.LINKSPEED",value);
            }
            xmlconfig->getValue("RADIO.CSMA",&value);//ON OFF
            if(value.length()==0)
            {
                value="ON";
                xmlconfig->setValue("RADIO.CSMA",value);
            }
            xmlconfig->getValue("RADIO.FEC",&value);//ON OFF
            if(value.length()==0)
            {
                value="OFF";
                xmlconfig->setValue("RADIO.FEC",value);
            }
            xmlconfig->getValue("RADIO.SCRAMBLER",&value);//ON OFF
            if(value.length()==0)
            {
                value="ON";
                xmlconfig->setValue("RADIO.SCRAMBLER",value);
            }

            xmlconfig->getValue("RADIO.AREA",&value);//9-eu
            if(value.length()==0)
            {
                value="9";
                xmlconfig->setValue("RADIO.AREA",value);
            }
        }
        /*else if(strstr(str,"SATEL")>0)
        {
            SysParam.radio=1;
            xmlconfig->setValue("RADIO.INFO.MODEL","SATEL");
            SysParam.direct_uhf=1;
            xmlconfig->setValue("RADIO.AVAILABLE_MODE","0|1|2|3");
            xmlconfig->setValue("RADIO.FREQ_UPPER_LIMIT","473");
            xmlconfig->setValue("RADIO.FREQ_LOWER_LIMIT","403");

            xmlconfig->getValue("RADIO.FEC",&value);//ON OFF
            if(value.length()==0)
            {
                value="OFF";
                xmlconfig->setValue("RADIO.FEC",value);
            }
        }*/
        else
        {
			xmlconfig->setValue("RADIO.INFO.MODEL","");
			xmlconfig->setValue("PORTS.RADIO.ENABLE","NO");
        }

        int file = open("/tmp/devinfo.txt", O_RDWR|O_CREAT,0666);
        if (file > 0)
        {
            write(file, tmp,strlen((char*)tmp));
            close(file);
        }
    }
    else
    {
        get_dev_serial(&serial);
        xmlconfig->setValue("DEVICE.INFO.SERIAL", serial);
        string model;
        memset(str,0,sizeof(str));
        strncpy(str,serial.c_str(),5);
        model.assign(str);
        xmlconfig->setValue("DEVICE.INFO.MODEL", model);

        SysParam.radio=0;
        SysParam.net=1;
        SysParam.direct_uhf=1;
    }


/*
    if(strstr(serial.c_str(),"NET20")>0)//NET20
    {
        xmlconfig->setValue("DEVICE.INFO.MODEL", "NET20");
    }
*/
    string os;
    string uboot;
    string uimage;
    get_sys_versions(&os, &uboot, &uimage);
    xmlconfig->setValue("DEVICE.INFO.OSVERSION", os);

    string imei;
    get_sys_imei(&imei);
    xmlconfig->setValue("DEVICE.INFO.IMEI", imei);
    printf("IMEI: %s",imei.c_str());

    //get_version_string(uboot,&value);
    value.assign(uboot);
    xmlconfig->setValue("DEVICE.INFO.BOOTVERSION", value);
    //get_version_string(APP_VERSION,&tmp);
    value.assign(APP_VERSION);
#ifndef CN_VERSION
    value.append("(foreign)");
#endif
    xmlconfig->setValue("DEVICE.INFO.APPVERSION", value);
     //get_version_string(WEB_VERSION,&tmp);
    value.assign(WEB_VERSION);
    xmlconfig->setValue("DEVICE.INFO.WEBSITEVERSION", value);

    string mac;
    get_mac(&mac);
    xmlconfig->setValue("DEVICE.INFO.MAC", mac);
#endif
    string size,free;
    get_disk_size(RECORD_DISK,&size,&free);
    xmlconfig->setValue("DEVICE.DATA_MEMORY.TOTAL", size);
    xmlconfig->setValue("DEVICE.DATA_MEMORY.FREE", free);
    get_disk_size("/root",&size,&free);
    xmlconfig->setValue("DEVICE.INTERNAL_MEMORY.TOTAL", size);
    xmlconfig->setValue("DEVICE.INTERNAL_MEMORY.FREE", free);

#ifdef CheckRegi
if(checkreg==1)
{
    checkauthcode = check_authcode((char *)serial.c_str(),&SysParam.ExpireDateTime,&SysParam.DeviceOption);
    value.assign((char *)authcode);
    xmlconfig->setValue("SYSTEM.AUTHCODE",value);
    if(checkauthcode)
    {
        printf("ExpireDateTime = %d\n", SysParam.ExpireDateTime);
        printf("DeviceOption = %x\n", SysParam.DeviceOption);

        sprintf(str,"%d",SysParam.ExpireDateTime);
        value.assign(str);
        xmlconfig->setValue("SYSTEM.EXPIREDATE",value);

        sprintf(str,"%d",SysParam.DeviceOption);
        value.assign(str);
        xmlconfig->setValue("SYSTEM.OPTION",value);

        xmlconfig->setValue("SYSTEM.STATE","CHECKING");
        SysParam.reg_state=REG_CHECKING;
    }
    else
    {
        SysParam.ExpireDateTime=0;
        SysParam.DeviceOption=0;
        xmlconfig->setValue("SYSTEM.EXPIREDATE","0");
        xmlconfig->setValue("SYSTEM.OPTION","0");
        xmlconfig->setValue("SYSTEM.STATE","ERROR");
        SysParam.reg_state=REG_ERROR;
    }
}
else
{
    SysParam.reg_state=REG_OK;
    xmlconfig->setValue("SYSTEM.EXPIREDATE","NONE");
    xmlconfig->setValue("SYSTEM.STATE","NORMAL");
    xmlconfig->setValue("DEVICE.STATUS","0");
}
#endif
    xmlconfig->saveConfig();
    return true;
}

string&   command::replace_all(string&   str,const   string&   old_value,const   string&   new_value)
{
    while(true)
    {
        string::size_type   pos(0);
        if(   (pos=str.find(old_value))!=string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }

    return  str;
}

int command::send_email(string title,string content)
{
        string cmd;
        string smtp,from,user,pass,to,ssl,port;
        if (xmlconfig->getValue("SYSTEM.EMAIL.SMTP",&smtp)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.FROM",&from)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.USER",&user)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.PASS",&pass)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.TO",&to)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.SSL",&ssl)!= RET_OK) return false;
        if (xmlconfig->getValue("SYSTEM.EMAIL.PORT",&port)!= RET_OK) return false;

        if(process[PROCESS_EMAIL]) stop_process(process[PROCESS_EMAIL]);

        pid_t pid;
        pid=fork();
        if (pid == -1)
        {
            perror("fork() error");
            syslog(LOG_LOCAL7|LOG_ERR,"cmd send_email fork()  error!");
            return RET_EXEC_FAILED;
        }

        content = replace_all(content,"&","%26");//URL特殊符号及编码
        content = replace_all(content,"$","%24");

        cmd.assign(EMAIL_SCRIPT);
        cmd=cmd+" "+smtp+" "+from+" "+user+" "+pass+" "+to+" \""+title+"\" "+" \""+content+"\" "+ssl+" "+port;

        if (pid == 0)
        {
            system(cmd.c_str());
            exit(0);
        }
        if (pid > 0)
        {
            process[PROCESS_EMAIL]=pid ;
        }
        return RET_OK;
}

int command::send_sms(string content)
{
        string cmd;
        string num;
        if (xmlconfig->getValue("SYSTEM.SMS.NUMBER",&num)!= RET_OK) return false;

        if(process[PROCESS_SMS]) stop_process(process[PROCESS_SMS]);

        pid_t pid;
        pid=fork();
        if (pid == -1)
        {
            perror("fork() error");
            syslog(LOG_LOCAL7|LOG_ERR,"cmd send_sms fork()  error!");
            return RET_EXEC_FAILED;
        }

        cmd.assign(SMS_SCRIPT);
        cmd=cmd+" ttyUSB/gprs0 "+num+"  \""+content+"\" ";

        if (pid == 0)
        {
            system(cmd.c_str());
            exit(0);
        }
        if (pid > 0)
        {
            process[PROCESS_SMS]=pid ;
        }
        return RET_OK;
}

bool command::add_ipc_data(char * szIPCWriteInfo,unsigned char *data,int len)
{
	int size8K=1;
	IPCMemoryINFO pIPCRadioCmd;

	if(AllocSHMEx(&pIPCRadioCmd,&size8K,szIPCWriteInfo)!=0)
	{
		return false;
	}

	printf("write memory %s addr:%x\n",szIPCWriteInfo,&(pIPCRadioCmd.IPCFIFO));

	int i,m,n;
	int writen;
	m=len/(FIFO_8K_SIZE-512);
	writen=0;
	for(i=0; i<=m; i++)
	{
		n=min(len-writen,FIFO_8K_SIZE-512);
		int ret = FIFO_Add_Buffer(&(pIPCRadioCmd.IPCFIFO),data+writen,n);
		//printf("add message  %d ,actual: %d, total:%d!\n",n,ret,len);
		usleep(100*1000);
		writen+=ret;
	}

	if (pIPCRadioCmd.iIPCMId!=INVALID_IPC_ID)
	{
		if(FreeSHM(&pIPCRadioCmd)!=0)
		{
			printf("free pIPCRadioCmd failed\n");
		}
		pIPCRadioCmd.iIPCMId=INVALID_IPC_ID;
	}

	return true;
}

//send command to "write" memory and get response from "read" memory, normally read and write id is set to process radio
//bool command::ipc_command(int read,int iReadSizeOf8K,int write,int iWriteSizeOf8K,unsigned char *data,int len,string *ret,int timeout_sec)
bool command::ipc_command(int IPCCmd,int IPCCmdResponse,unsigned char *data,int len,string *ret,int timeout_sec)
//bool command::Radio_command(unsigned char *data,int len,string *ret,int timeout_sec)
{
    int n;
    char buf[1025];
    int length;
    //int shm_w;
    //FIFO fifo_r;
    //FIFO fifo_w;
	IPCMemoryINFO *pIPCRadioCmd;
	IPCMemoryINFO *pIPCRadioCmdResponse;
	std::map<int, IPCMemoryINFO *>::iterator it;
    ret->clear();
    //command ipc
	it= m_mapIPCCmd.find(IPCCmd);
	if (it == m_mapIPCCmd.end())
	{
		pIPCRadioCmd = new IPCMemoryINFO();
		if(AllocSHM(pIPCRadioCmd,IPCCmd,IPC_CMD_SIZE8K)!=0)
		{
			free(pIPCRadioCmd);
			return false;
		}
		m_mapIPCCmd[IPCCmd]=pIPCRadioCmd;
		FIFOInitForWriting(&(pIPCRadioCmd->IPCFIFO),FIFOWrite_SIZE(IPC_CMD_SIZE8K));
	}
	else
	{
		pIPCRadioCmd=it->second;
	}
	//printf("ipc_command: cmd fifo id %d\n",pIPCRadioCmd->iIPCMId);
	it= m_mapIPCCmd.find(IPCCmdResponse);
	if (it == m_mapIPCCmd.end())
	{
		pIPCRadioCmdResponse = new IPCMemoryINFO();
		if(AllocSHM(pIPCRadioCmdResponse,IPCCmdResponse,IPC_CMD_SIZE8K)!=0)
		{
			free(pIPCRadioCmdResponse);
			return false;
		}
		m_mapIPCCmd[IPCCmdResponse]=pIPCRadioCmdResponse;
	}
	else
	{
		pIPCRadioCmdResponse=it->second;
	}

    FIFOInitForReading(&(pIPCRadioCmdResponse->IPCFIFO));
	//printf("ipc_command: cmd response fifo id %d\n",pIPCRadioCmdResponse->iIPCMId);
	//printf("ipc_command: cmd %s\n",data);

	FIFO_Add_Buffer(&(pIPCRadioCmd->IPCFIFO),data,len);

    for(n=0; n<(timeout_sec*2); n++)
    {
        usleep(1000*500);
        if(n>10)clear_msg(PROC_INTERFACE);
        if (FIFO_Get_Length(&(pIPCRadioCmdResponse->IPCFIFO)) > 0 )
        {
            usleep(1000*5);
            break;
        }
    }

    while(FIFO_Get_Length(&(pIPCRadioCmdResponse->IPCFIFO)) > 0)
    {
        length=FIFO_Get_Buffer(&(pIPCRadioCmdResponse->IPCFIFO),(unsigned char *)buf,sizeof(buf)-1);
        if(length>0)
        {
            buf[length]=0;
            ret->append(buf);
        }
    }


    return true;
}


void command::CMDSendMessage(long target,string str)
{
    MSG_BUFF msg;
    msg.msg_type = target;
    msg.msg_source = PROC_INTERFACE;
	//syslog(LOG_LOCAL7|LOG_INFO,"add message  %d!",str.length());

	char szIPCWriteInfo[20]={0};
	if(target==CMD_BT_ID)
	{
		sprintf(szIPCWriteInfo,"%s",IPC_BT_WRITE_INFO);
	}
	else if(target==CMD_COM_ID)
	{
		sprintf(szIPCWriteInfo,"%s",IPC_COM_WRITE_INFO);
	}
	else if(target==CMD_COM2_ID)
	{
		sprintf(szIPCWriteInfo,"%s",IPC_COM2_WRITE_INFO);
	}
	else if(target==CMD_COM3_ID)
	{
		sprintf(szIPCWriteInfo,"%s",IPC_COM3_WRITE_INFO);
	}
	else if(target==CMD_TCP_ID)
	{
		sprintf(szIPCWriteInfo,"%d-%d",IPC_TCP_WRITE,TCP_CMD_SIZE8K);
	}
	else if(target==CMD_TCP1_ID)
	{
		sprintf(szIPCWriteInfo,"%d-%d",IPC_TCP1_WRITE,TCP_CMD_SIZE8K);
	}
	else if(target==CMD_TCP2_ID)
	{
		sprintf(szIPCWriteInfo,"%d-%d",IPC_TCP2_WRITE,TCP_CMD_SIZE8K);
	}
	else if(target==CMD_REMOTE_ID)
	{
		sprintf(szIPCWriteInfo,"%s",IPC_REMOTE_READ_INFO);
	}
	if(strlen(szIPCWriteInfo)>0)
	{
		add_ipc_data(szIPCWriteInfo,(unsigned char *)str.c_str(),str.length());
	}
	else
	{
		if(/* interface_qid>0 && */str.length()>0)
		{
			int i,n,m,len,size,writen;
			const char *p=str.c_str();
			size=str.length();
			m=size/1000;
			writen=0;
			for(i=0; i<=m; i++)
			{
				n=min(size-writen,1000);
				strncpy(msg.msg_text,p+writen,n);
				msg.msg_text[n]=0;
				len = n+ 5;
				//printf("len %d\r\n",len);
				if ((msgsnd(interface_qid, &msg, len, 0)) < 0)
				{
					//printf("qid: %d len:%d",qid, len);
					syslog(LOG_LOCAL7|LOG_ERR,"[CMDSendMessage]ERROR: errno = %d, strerror = %s \n" , errno, strerror(errno));
					//exit(1);
				}
				writen+=n;
			}
		}
	}
}

bool command::GetPortsRTCMStatus(const char *szPortXMLName)
{
	string enable,func;

	GetPortXMLValue("PORTS.%s.ENABLE",szPortXMLName,&enable);
	if(enable.compare("YES") == 0 )
	{
		GetPortXMLValue("PORTS.%s.FUNCTION",szPortXMLName,&func);
		if(func.compare("RTK_OUT") == 0 )
		{
			return true;
		}
	}

	return false;
}

bool command::checkPortsRTCMOut()
{
	int bRTCMOut=false;

	bRTCMOut = GetPortsRTCMStatus("BLUETOOTH");
	bRTCMOut += GetPortsRTCMStatus("RADIO");
	bRTCMOut += GetPortsRTCMStatus("COM1");
	bRTCMOut += GetPortsRTCMStatus("COM2");
	bRTCMOut += GetPortsRTCMStatus("COM3");
	bRTCMOut += GetPortsRTCMStatus("SOCKET");
	bRTCMOut += GetPortsRTCMStatus("SOCKET1");
	bRTCMOut += GetPortsRTCMStatus("SOCKET2");
	//syslog(LOG_LOCAL7|LOG_INFO,"checkPortsRTCMOut:%d",bRTCMOut);
	return (bRTCMOut!=false);
}

int command::do_GETALL(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;
	string msg;
	msg.clear();
    if(argc>0)
    {
        result = xmlconfig->getallWithNode(argv[1],msg);
    }
    else
    {
        result = xmlconfig->getall(msg,SysParam.radio);
    }

	if (result == RET_OK)
		ShowMsg(msg);
	//syslog(LOG_LOCAL7|LOG_INFO,"returnMsg:%d",returnMsg.length());
	return result;
}

