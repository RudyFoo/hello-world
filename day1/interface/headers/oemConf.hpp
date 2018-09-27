/*
 * oemConf.hpp
 */

#ifndef OEMCONF_HPP_
#define OEMCONF_HPP_

#include "share.hpp"
#include "fifo.h"
using namespace std;

enum  OEM_NMEA_MSG_TYPE{GGA=0,GSA,GSV,ZDA,GST,GLL,RMC,VTG,PJK};
enum  OEM_LOG_TRG_TPYE{TRIGGER_ONTIME = 0,TRIGGER_ONCHANGED,TRIGGER_ONCE,TRIGGER_NONE};
 typedef enum enumDiffType
  {
    RTCA=1,
    RTCM2,
    RTCM3,
    CMR,
    CMRPLUS,
    SCMRX,
    DGPS,
    NOVATELX,
    RTCM32MSN,
    ROX,
    RTCM3210,
    UNDEFINE_DIFFTYPE
  } DIFF_TYPE;

class OemCommand
{

public:
	OemCommand();
	virtual ~OemCommand();
    virtual bool init()=0;
    virtual bool issuenmea(unsigned char port,OEM_NMEA_MSG_TYPE msgtype,OEM_LOG_TRG_TPYE trigger,int interval)=0;
	virtual bool lognmea(unsigned char port, char *setting)=0;
	virtual bool unlogall(unsigned char port)=0;
	virtual bool rtkmode(unsigned char port)=0;
	virtual bool issueDiffData(unsigned char port, DIFF_TYPE type,double lat, double lon, float height,int id)=0;
	virtual bool startbase(double lat, double lon, float height,int id)=0;
	virtual bool stopbase()=0;
	virtual bool checkbasestarted()=0;
	virtual bool setOemVersion(bool ver7)=0;
	virtual bool issueRTCM23(unsigned char port,int id)=0;
	virtual bool issueRTCM30(unsigned char port,int id)=0;
	virtual bool issueRTCM32(unsigned char port,int id)=0;
	virtual bool issueRTCM3210(unsigned char port,int id)=0;
	virtual bool issueRTCMV3EPH(unsigned char port,string rtcmv3_eph)=0;
	virtual bool issueCMR(unsigned char port,int id)=0;
	virtual bool issueSCMRX(unsigned char port,int id)=0;
	virtual bool issueROX(unsigned char port,int id)=0;
	virtual bool issueDGPS(unsigned char port,double lat, double lon, float height,int id)=0;
    virtual bool issueRAW(unsigned char port,int interval,int raw_eph)=0;
    virtual bool issueRAWEPH(unsigned char port,int interval, int raw_eph)=0;
    virtual bool issueBINEX(unsigned char port,int interval, int raw_eph)=0;
	virtual bool setElemask(unsigned char elem)=0;
	virtual bool issueRecord(int interval)=0;
	virtual bool stopRecord()=0;
	virtual bool stopDiff(unsigned char port)=0;
	virtual bool setDefault()=0;
    virtual bool setGlonass(int disable)=0;
    virtual bool setBeidou(int disable)=0;
    virtual bool setGPS(int disable)=0;
    virtual bool setGalileo(int disable)=0;
	virtual bool setQzss(int disable)=0;
	virtual bool setSbas(int disable)=0;
	virtual bool disableprn(int prn)=0;
	virtual bool enableallprn()=0;
    virtual bool txtcommand(char *cmd)=0;
    int error_count;
	int oem7;
};

#endif
