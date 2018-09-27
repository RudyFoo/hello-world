/*
 * command.h
 *
 *  Created on: 2009-7-28
 *      Author: Administrator
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <map>
#include <iostream>
#include <string>
#include <syslog.h>
#include <xmlconf.hpp>
#include "keywords.hpp"

//OEM
#include "oemConf.hpp"
#include "TrimbleConf.hpp"
#include "NovConf.hpp"
#include "UnicoreConf.hpp"
#include "HemConf.hpp"

#include "util.hpp"

#define MAX_BUF_LEN		16384U	/* The Length of UART0 receive Buffer. */
#define MAX_CMD_ARGS	    5
#define MAX_CMD_NODE	    5
#define MAX_CMD_LEN		512
//#define NULL			0
#define MAX_PROCESS_ARGS  8

#define ARGTYPE		int
#define ARG_ENUM	0x00000001
#define ARG_INT		0x00000002
#define ARG_FLT		0x00000004
#define ARG_STR		0x00000008
#define ARG_OPT		0x00000020

#define RET_OK				0x00
#define RET_INVALID_CMD		0x01
#define RET_INVALID_NODE	0x02
#define RET_INVALID_VALUE	0x03
#define RET_INVALID_OPERATION		0x04	//invalid operation
#define RET_ERR_PARAM		0x05	//Param number not correct
#define RET_EXEC_FAILED	0x06
#define RET_UNSUPPORTED_NOW 0x07

#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))

#define MAX_PROCESS  100

typedef enum  {
	PROCESS_MAIN = 0,
    PROCESS_OEM=1,
    PROCESS_OLED,
	PROCESS_DIFF,
	PROCESS_REC,
	PROCESS_REC1,
	PROCESS_REC2,
	PROCESS_REC3,
	PROCESS_REC4,
	PROCESS_REC5,
	PROCESS_REC6,
	PROCESS_REC7,
	PROCESS_REC8,
	PROCESS_REC9,
	PROCESS_REC_TCP,
	//PROCESS_NTRIP,
	PROCESS_CMD,
	PROCESS_BT,
	PROCESS_BT_O,
	PROCESS_BT_BINEX,
	PROCESS_RADIO_O,
	PROCESS_RADIO_BINEX,
	PROCESS_COM,
	PROCESS_COM_O,
	PROCESS_COM_BINEX,
	PROCESS_COM2,
    PROCESS_COM2_O,
    PROCESS_COM2_BINEX,
	PROCESS_COM3,
	PROCESS_COM3_O,
	PROCESS_COM3_BINEX,
	PROCESS_TCP,
	PROCESS_TCP_O,
	PROCESS_TCP_BINEX,
	PROCESS_TCP1,
	PROCESS_TCP1_O,
	PROCESS_TCP1_BINEX,
	PROCESS_TCP2,
	PROCESS_TCP2_O,
	PROCESS_TCP2_BINEX,
	PROCESS_NTRIP_CLIENT,
	PROCESS_NTRIP_CLIENT_O,
	PROCESS_NTRIP_CASTER,
	PROCESS_FTP,
	PROCESS_EMAIL,
	PROCESS_SMS,
	PROCESS_SELF_CHECK,
	PROCESS_HUB,
	PROCESS_NTRIP,
	PROCESS_NTRIP_O,
	PROCESS_NTRIP1,
	PROCESS_NTRIP1_O,
	PROCESS_NTRIP2,
	PROCESS_NTRIP2_O,
	PROCESS_NTRIP3,
	PROCESS_NTRIP3_O,

    PROCESS_BATCHUPDATE,
    PROCESS_RADIO,
    PROCESS_UPGRADE,

    PROCESS_REMOTE,
	PROCESS_REMOTE_C,
	PROCESS_TRANSPOND,
}PROCESS;

typedef struct
{
    char type[32];
    char igs[300];
    char igr[300];
    float hl1;
    float hl2;
}ANTENNA_INFO;

class command;

typedef struct stCMDTxt {
	const char * cmdname;	// Command Name
	int			 minargs;	// Minimum number of arguments
	int			 maxargs;	// Maximum number of arguments
	int (command::*doCmd)(int argc, string (&argv)[MAX_CMD_ARGS]);
							// Implementation function
	const char * usage;		// Command Usage
} TXT_CMD;

typedef struct stArg {
	ARGTYPE		   type;	// Argument type
	const char *   argname;	// Argument name
	const char **  enumargv;// ENUM type argument list. null-terminated.
							// If not a ENUM type argument, set to NULL.
	const char *   defargv;	// Default value of OPTIONAL type argument.
							// If not a OPTIONAL type argument, set to NULL.
	struct stArg * nextarg;	// next argument
	struct stArg * subarg;	// sub-argument
	int			   data;	// result...
} TXT_CMD_ARG;

typedef enum  {
    PORT_UNKNOWN=0,
    PORT_SERIAL=1,
    PORT_SOCKET=2,
	PORT_ICOM=3,
}PORT_TYPE;

typedef struct _ENDPOINT_OPT{
     const char *target;
     PORT_TYPE type;
     int id;
     int option;
     int pid;
}ENDPOINT_OPT;

typedef struct _RECORDSESSION_OPT{
    char name[128];
    int key;
    int id;
    int option;
}RECORDSESSION_OPT;

typedef struct _TIMEZONE_TABLE{
	char zoneinfo[128];
	int zone;
}TIMEZONE_TABLE;

typedef enum enumRegState
  {
    REG_IDLE,
    REG_ERROR,
    REG_CHECKING,
    REG_EXPIRED,
    REG_OK
  } REG_STATE_TYPE;

#pragma pack(8)
typedef struct  _tagSYSPARAM {
    REG_STATE_TYPE reg_state;
    unsigned int NowDateTime;
    unsigned int ExpireDateTime;
    unsigned char DeviceOption;
    int radio;
	int net;
	int direct_uhf;
	int uhf_enable;
	int hardver;
}tagSysParam;
#pragma pack()

class command
{
public:
	command(int gpsboard);
	~command();
	void CMDParse(string strCMD,long source);
	string returnMsg;
	config* xmlconfig;
	OemCommand *oem;
	int gpsboard;
	int littlegprsboard;
    int bluetooth_enable;
	int wifi_enable;
	int com1_baud;
	int com2_baud;
	int com3_baud;
	int ntrip_connection;
	int record_enable;
	int base_started;
    int use_raw_port;
    int direct_link;
    int rs485_flag;
    int checkreg;
    int usb_connected;
    int ntrip_double;
    int interface_qid;
    int recSessionEmpty;
	int cur_cpld_mode;
	int last_cpld_mode;
	int ntp_enable;
    unsigned int oem_msg_counter;
    tagSysParam SysParam;
    string param;
    //ucshell
    string ucshell_cmd;
    string ucshell_param;
    long ucshell_source;
    pthread_t ucshell_thread;
private:
	TXT_CMD* FindCMD(string cmd);
	pid_t  process[MAX_PROCESS];
	int ParseLine(string line, string (&argv)[MAX_CMD_ARGS]);
	void ShowMsg(const string cmd);
	void ShowMsgLine(const string str);
	string AssemblyMsgLine(const string str);

	int GetEnumArgIndex(const char *enumv[], const char *argv);
	int GetDigitFromString(const string cmd);
	//int CheckArgs(TXT_CMD_ARG *pArgs, int argc, char *argv[]);
	//int Traverse(TXT_CMD_ARG *pArg, char **argv[], bool IsSetDef);
	int node[MAX_CMD_NODE];
	int nodes;
	long cmd_source;
    string curCommand;

	int DisableSecurityMode;
	static TXT_CMD CmdTable[];
	int do_GET(int argc, string (&argv)[MAX_CMD_ARGS]);
	int do_SET(int argc, string (&argv)[MAX_CMD_ARGS]);
	int do_LIST(int argc, string (&argv)[MAX_CMD_ARGS]);
	int do_SAVE(int argc, string (&argv)[MAX_CMD_ARGS]);
	int do_UPDATE(int argc, string (&argv)[MAX_CMD_ARGS]);
    int do_GETALL(int argc, string (&argv)[MAX_CMD_ARGS]);
    int do_DEL(int argc, string (&argv)[MAX_CMD_ARGS]);
    int do_ADD(int argc, string (&argv)[MAX_CMD_ARGS]);
    bool SetCmdSource(long source);
    bool start_process(int argc, char** argv, pid_t &process_id);
    bool stop_process(pid_t &process_id);
    bool init_endpoint();
    bool count_endpoint();
    bool creat_endpoint_nov(ENDPOINT_OPT *endpoint, int only_socket);
    bool change_endpoint(ENDPOINT_OPT *endpoint,int process);
    //bool creat_endpoint(ENDPOINT_OPT *endpoint, int index, int only_socket, int read,int write);

	//readIPCInfo:the GNSS board commmands buffer to send
	//writeIPCInfo:the buffer read from gnss board
	bool creat_endpoint(ENDPOINT_OPT *endpoint, int index, int only_socket,const char * readIPCInfo,const char * writeIPCInfo);
    bool release_endpoint(ENDPOINT_OPT *endpoint, int index);
    bool find_endpoint(ENDPOINT_OPT *endpoint, int index);
    bool init_recsession();
    bool creat_recsession(RECORDSESSION_OPT *session,char *name);
    bool release_recsession(RECORDSESSION_OPT *session,char *name);
    bool count_recsession();
	bool checkPortsRTCMOut();
	void ctl_setDataLink(int mode);
    void default_cpld_mode();
	void set_sysTimezone(int timezone);
	int Dowork();
    int Del_Dowork();
    int Del_Ntrip();
    int Del_Ntrip_Connection();
    int Del_Record();
    int Del_Record_Session();
	//nat
    int Del_NAT();
    int Del_Dnat();
    int Del_Snat();
    int net_snat();
    int net_dnat();
    int net_snat_Reset();
    int net_dnat_Reset();

	//IPCMemoryINFO m_IPCRadioCmdRead;
	//IPCMemoryINFO m_IPCRadioCmdWrite;
	map<int, IPCMemoryINFO *> m_mapIPCCmd;

	bool GetPortsRTCMStatus(const char *szPortXMLName);
	int GetPortXMLValue(const char * szXMLPattern,const char * szPortXMLName,string *value);
	//
	void StartProcess_cmd(const char * szIPCReadInfo,const char * szIPCWriteInfo,int iMailboxId,pid_t &process_id);
	//szIPCReadInfo: where serial2 read cmd from,will send to gnss board
	//szIPCWriteInfo:where serial2 write response to,the data from gnss board
	void StartProcess_Serial2(const char * COMName,const char * baudrate,const char * szIPCReadInfo,const char * szIPCWriteInfo,pid_t &process_id);

	void StartProcess_Met(const char * COMName,const char * baudrate,const char * szIPCWriteInfo,pid_t &process_id);

	//read DIFF data from gnss board to szIPCWriteInfo
	int StartRTKOUTPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
		const char *szIPCReadInfo,const char *szIPCWriteInfo);

	//endpoint transfer data from gnss to szIPCWriteBinexInfo,
	//then toBinex redecorate data and transfer them to szIPCWriteInfo
	int StartBinexPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,
		int iEPProcIdIndex0,const char *szIPCReadInfo,const char * szIPCWriteInfo,
		int iBinexProcIdIndex,const char *szIPCWriteBinexInfo,string option,int iCheckEPH);

	//szIPCReadInfo:where cmd/diff data read from
	//szIPCWriteInfo:where cmd write response/gnss data or gnss data
	//return 0:func is found, but process failed; 1:func is found, process sucessful; -1: func is not found
	int StartPortWithConfigedFunction(const char * szPortXMLName,string func,ENDPOINT_OPT * endpoint,
		int iEPProcIdIndex0,const char *szIPCReadInfo,const char * szIPCWriteInfo,
		int iBinexProcIdIndex,const char * szIPCWriteBinexInfo,
		int iMailboxId,string option,int iCheckEPH);

	//szIPCReadInfo: where program server read data from, the source buffer to send to socket
	//szIPCWriteInfo: where program server write to,the dest buffer to write the data from socket
	/*void SetupSocketPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
		int iTCPProcIdIndex,const char *szIPCReadInfo,const char * szIPCWriteInfo,
		int iBinexProcIdIndex,const char * szIPCWriteBinexInfo, int iMailboxId,string option);*/
	void SetupSocketPort(const char * szPortXMLName,ENDPOINT_OPT * endpoint,int iEPProcIdIndex0,
		int iTCPProcIdIndex,int iIPCRead,const char * szIPCWriteInfo,
		int iBinexProcIdIndex,int iIPCBinex, int iMailboxId,string option);

	//if COM1 enable ntripdouble, start it
	void CheckAndStartNtripDoubleInCOM1(const char * szIPCNtripSource);
public:
    void clear_msg(unsigned int type);
    bool init_gps();
    bool init_oled();
    bool init_oled_upgrade();
    bool init_dev();
    bool default_setting();
    bool gps_default_setting();
    bool get_infos();
#ifdef ANTENNA_MANAGEMENT
    int initAntennaModel();
    int setBaseAntennaModel();
#endif
	int Device();
	int Record();
	int dev_rec_Startrec();
	int dev_rec_Stoprec();
	int set_record_start(string name);
	int set_record_stop(string name);
	int dev_rec_Onchanged();
	int dev_rec_Delete();
	int dev_rec_push();
	int dev_rec_ftp_Push();
	int dev_rec_geo_Push();
	int dev_rec_gCompress();
	int dev_ReStartBase();
    int dev_StartBase();
	int dev_StopBase();
    int dev_Reset();
    int dev_Regi();
    int dev_SecurityCode();
    int dev_Devinfo();
    int dev_PowerOff();
    int dev_PowerLevel();
    int dev_CleanLog();
    int dev_SelftCheck();
    int dev_Freset();
    int dev_Format();
    int dev_BatchUpdate();
    int dev_Update();
    int dev_Remote();
    int dev_Remote_Reset();
    int dev_Upgrade();
    int dev_Ucshell();
    int dev_Configset();
    int dev_Configset_Save();
    int dev_Configset_Restore();

    int Disk();
    int disk_MountMSD();

	int Gps();
	int gps_DisablePrn();
	int gps_EnableAllPrn();
	int set_gps_disableprn(string opt);
	int Ntrip();
	int ntrip_Reset();
	int ntrip_connect();
	int ntrip_disconnect();
    int set_ntrip_connect(int id);
    int set_ntrip_disconnect(int id);

    int Network();
    int net_priority();
    int net_priority_Reset();
    int net_wan();
    int net_wan_Reset();
    int net_wan_Connected();
	int net_wan_IP();
    int net_wifi();
    int net_wifi_Reset();
    int net_wifi_Connect();
    int net_wifi_Scan();
	int net_wifi_IP();
    int net_gprs();
    int net_gprs_Reset();
    int net_gprs_Status();
	int net_gprs_IP();
	int net_dns1();
    int net_dns1_Reset();
    int net_ftp();
    int net_ftp_Reset();
	int net_ntp();
	int net_ntp_Reset();
	int net_DynamicDns();
	int net_DynDns_Enable();
	int net_set_ddns();

	int Ports();
	int port_Reset();
	int port_Bluetooth();
	int port_BluetoothReset();
    int port_BluetoothEnable();
    int port_BluetoothConnected();
    int port_socket(int num);

    int System();
    int sys_Default();
    int sys_Email();
    int sys_EmailTest();
    int sys_Sms();
    int sys_SmsTest();
    int sys_HttpPort();
	int sys_Timezone();
	int sys_Tzid();

    int Radio();
    int radio_frequency();
    int radio_channel();
    int radio_power();
    int radio_mode();
    int radio_Reset();
    int radio_change_channel(int channel);
    int radio_selfcheck();

    int Vpn();
	int vpn_Reset();
	int vpn_Nari();
	int vpn_NariReset();

    //func
    string &replace_all(string&   str,const   string&   old_value,const   string&   new_value);
    int send_email(string title,string content);
    int send_sms(string content);
	bool add_ipc_data(char * IPCCmd,unsigned char *data,int len);
	//bool ipc_command(int read,int write,unsigned char *data,int len,string *ret,int timeout_sec);
	bool ipc_command(int IPCCmd,int IPCCmdResponse,unsigned char *data,int len,string *ret,int timeout_sec);
	//bool Radio_command(unsigned char *data,int len,string *ret,int timeout_sec);

    void CMDSendMessage(long target, string str);
	bool stopSubProcess();
};

#endif /* COMMAND_H_ */
