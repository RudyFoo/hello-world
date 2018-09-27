/*
 * NovConf.hpp
 *
 *  Created on: 2009-8-29
 *      Author: zekunyao
 */

#ifndef NOVCONF_HPP_
#define NOVCONF_HPP_

#include <string>
#include "share.hpp"
#include "oemConf.hpp"
#include "fifo.h"
#include "IPCInfoUtil.h"

using namespace std;
class cNovatelCommand: public OemCommand
{
private:
	string logPrefix;
	char checkStr[32];
	bool result;
	int cmdmailbox;
	MSG_MAILBOX mail;
	//int shm_w;
 //   FIFO fifo_w;
	IPCMemoryINFO m_IPCGPSRAWWrite;
    int base_started;
	bool sendOnce(string cmd);
public:
	cNovatelCommand();
	~cNovatelCommand();
	bool getResult();
	void resetResult();
	bool sendcmd(string cmd);

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
	bool issueRTCM32(unsigned char port, int id);
	bool issueRTCM3210(unsigned char port, int id);
	bool issueRTCMV3EPH(unsigned char port,string rtcmv3_eph);
    bool issueROX(unsigned char port, int id);
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
	bool init();
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
