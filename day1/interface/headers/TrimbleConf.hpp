/*
 * TrimbleConf.hpp
 */

#ifndef TRIMBLECONF_HPP_
#define TRIMBLECONF_HPP_

#include <string>
#include "share.hpp"
#include "oemConf.hpp"
#include "fifo.h"
#include "IPCInfoUtil.h"

#define OEM_COMMAND_BUFFER_LENGTH 2048
#define OEM_APPFILE_BUFFER_LENGTH  2048

#define OEM_PORT_STATIC  21 //tcp 5018
#define OEM_PORT_DIFF 	20 //tcp 5017
#define OEM_PORT_COMMAND	1

#define 	STX		0x02
#define 	ETX		0x03
#define 	ENQ		0x05
#define 	ACK        0x06
#define 	NAK		0x15
#define PI 3.1415926535898

#define  OEM_HTTP "http://admin:password@192.167.100.190/"

#define SET_PORT_TCP_5017  (char *)"cgi-bin/io.xml?port=20&LocalPort=5017&type=TCP%2FIP&OutputOnly=off"

#define ADD_PORT_TCP_5020  (char *)"cgi-bin/io.xml?port=29&type=TCP%2FIP&LocalPort=5020&Add=1"
#define ADD_PORT_TCP_5021  (char *)"cgi-bin/io.xml?port=28&type=TCP%2FIP&LocalPort=5021&Add=1"
#define ADD_PORT_TCP_5022  (char *)"cgi-bin/io.xml?port=27&type=TCP%2FIP&LocalPort=5022&Add=1"
#define ADD_PORT_TCP_5023  (char *)"cgi-bin/io.xml?port=26&type=TCP%2FIP&LocalPort=5023&Add=1"
//#define ADD_PORT_TCP_5023  "cgi-bin/trackingPage.xml?elevMask=0&Everest=Enabled&ClockSteering=Disabled&L1CA_enable=on&L2E_enable=on&L2E_fallback=0&L2C_enable=on&L2C_mode=CM_CL&L5_enable=on&L5_mode=I_Q&G1C_enable=on&G1P_enable=on&G2C_enable=on&G2C_fallback=0&G3_mode=D_P&E1_enable=on&E1_mode=D_P&E1_mboc=on&E5A_enable=on&E5A_mode=D_P&E5B_enable=on&E5B_mode=D_P&E5AltBOC_enable=on&E5AltBOC_mode=D_P&B1_enable=on&B2_enable=on&QZSSL2C_mode=CM_CL&QZSSL5_mode=I_Q"
#define ENABLE_1PPS (char *)"cgi-bin/generalPage.xml?PPSEnable=Enabled"
#define DISABLE_1PPS (char *)"cgi-bin/generalPage.xml?PPSEnable=Disabled"

#define ENABLE_EVENT (char *)"cgi-bin/generalPage.xml?Event1Enable=Enabled"
#define DISABLE_EVENT (char *)"cgi-bin/generalPage.xml?Event1Enable=Disabled"
#define EVENT_NEGATIVE (char *)"cgi-bin/generalPage.xml?Event1Enable=Enabled&Event1Slope=Negative"
#define EVENT_POSITIVE (char *)"cgi-bin/generalPage.xml?Event1Enable=Enabled&Event1Slope=Positive"

#define SET_TRACKING (char *)"cgi-bin/trackingPage.xml?elevMask=0&Everest=Enabled&ClockSteering=Disabled&L1CA_enable=on&L2E_enable=on&L2E_fallback=0&L2C_enable=on&L2C_mode=CM_CL&L5_enable=on&L5_mode=I_Q&G1C_enable=on&G1P_enable=on&G2C_enable=on&G2P_enable=on&G2P_fallback=0&G3_mode=D_P&E1_enable=on&E1_mode=D_P&E1_mboc=on&E5A_enable=on&E5A_mode=D_P&E5B_enable=on&E5B_mode=D_P&E5AltBOC_enable=on&E5AltBOC_mode=D_P&B1_enable=on&B2_enable=on&QZSSL1CA_enable=on&QZSSL2C_enable=on&QZSSL2C_mode=CM_CL&QZSSL5_enable=on&QZSSL5_mode=I_Q"

typedef struct stoemcommandpara  //
{
 // int msgid;
  unsigned char data[OEM_COMMAND_BUFFER_LENGTH];
  int length;
}OEM_COMMAND;

typedef struct stoemappbuf  //
{
  unsigned char data[OEM_APPFILE_BUFFER_LENGTH];
 // int msgid;
  int index;
  int length;
}OEM_APPBUF;

using namespace std;

class cTrimbleCommand: public OemCommand
{
private:
	string logPrefix;
	char checkStr[32];
	bool result;
	int base_started;
	int cmdmailbox;
	MSG_MAILBOX mail;
	//int shm_w;
 //   FIFO fifo_w;
	IPCMemoryINFO m_IPCGPSRAWWrite;
	OEM_COMMAND command;
	OEM_APPBUF  appbuf;
	bool gpsdisabletab[32];
	bool glonassdisabletab[32];
	bool beidoudisabletab[30];
	bool galileodisabletab[36];

	void countsum();
	void swap8(void * ptr);
	void swap4(void * ptr);
	void swap2(void * ashort);
    void commandgetserial();
	void clearappfilerecords();
	bool applystopallrecord(unsigned char port);
	bool applygeneralcontrolsrecord(unsigned char elevMask, unsigned char pdopMask);
	bool applyrt17record(unsigned char port, unsigned char frequency);
	bool applybinexrecord(unsigned char port, unsigned char frequency);
    bool applyreferencenoderecord(char* name, double lat, double lon, double height, int rtcmid, int cmrid);
    bool applyrtcmrecord(unsigned char port, unsigned char frequency, unsigned char offset, unsigned char rtcmflag, unsigned char rtcmflag2);
    bool applycmrrecord(unsigned char port, unsigned char frequency, unsigned char offset, unsigned char msgtype);
    bool applynmearecord(unsigned char port, unsigned char frequency, unsigned char offset, unsigned char type);
	bool applynmeav41(int v41,char port, char frequency, char offset, char type);
	bool applyqzssrecord(int disable);
	bool applyglonassrecord(int disable);
	bool applybeidourecord(int disable);
	bool applygpsrecord(int disable);
	bool applygalileorecord(int disable);
	bool applygsbasrecord(int enable);
	bool disablegpssnr(int snr);
	bool disableglonasssnr(int snr);
	bool disablegalileosnr(int snr);
	bool disablebeidousnr(int snr);
	bool commandappfile();
	void sendcommand();
	bool sendbincommand(bool isappfile);
    bool GetHttp(const char *url, const char *filename);
public:
	cTrimbleCommand();
	~cTrimbleCommand();
	bool getResult();
	void resetResult();
	bool send(string cmd);
    bool init();
    int getMsgid(OEM_NMEA_MSG_TYPE msgtype);
    int getMsgfreq(int interval);
    bool issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval);
	bool lognmea(unsigned char port, char *setting);
	bool unlogall(unsigned char port);
	bool unlogallports();
	bool rtkmode(unsigned char port);
	bool issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height, int id);
	bool startbase(double lat, double lon, float height, int id);
	bool stopbase();
	bool checkbasestarted();
	bool setOemVersion(bool ver7);
	bool issueRTCM23(unsigned char port, int id);
	bool issueRTCM30(unsigned char port, int id);
	bool issueROX(unsigned char port, int id);
	bool issueRTCM32(unsigned char port, int id);
	bool issueRTCM3210(unsigned char port, int id);
	bool issueRTCMV3EPH(unsigned char port,string rtcmv3_eph);
	bool issueCMR(unsigned char port, int id);
	bool issueSCMRX(unsigned char port, int id);
	bool issueDGPS(unsigned char port,double lat, double lon, float height, int id);
	bool issueRAW(unsigned char port,int interval, int raw_eph);
	bool issueRAWEPH(unsigned char port,int interval, int raw_eph);
	bool issueBINEX(unsigned char port,int interval, int raw_eph);
	bool setElemask(unsigned char elem);
	bool issueRecord(int interval);
	bool stopRecord();
	bool stopDiff(unsigned char port);
	bool setDefault();
    bool setGlonass(int disable);
    bool setBeidou(int disable);
    bool setGPS(int disable);
    bool setGalileo(int disable);
	bool setQzss(int disable);
	bool setSbas(int disable);
	bool disableprn(int prn);
	bool enableallprn();
    bool txtcommand(char *cmd);
};

#endif /* NOVCONF_HPP_ */
