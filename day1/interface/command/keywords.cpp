#include <stdio.h>
#include <string.h>
#include "keywords.hpp"

const char * PATHTABLE[]=
{
    "DEVICE","SYSTEM","ANTENNA","GPS","RADIO","NETWORK","DISK","EXTERNAL","BLUETOOTH","ENABLE","CONNECTED","WIFI","SCAN","GPRS",\
	"NTRIP","RECORD","PORTS","SELF_CHECK","STARTBASE","STOPBASE","RESET","POWEROFF","POWER_LEVEL","STARTRECORD","STOPRECORD",\
	"CONNECT","DISCONNECT","WAN","MOUNTMSD","CLEANLOG","FTP","PUSH","GEO","TEST","EMAIL","SMS","HTTP_PORT","DEFAULT","NTRIP_CLIENT","CONNECTION","DELETE",\
	"SECURITY_CODE","DEVINFO","REGI","SOCKET0_RAW","SOCKET1_RAW","SOCKET2_RAW","UPDATE","MODE","POWER","CHANNEL","FREQUENCY","UPGRADE","FRESET","FORMAT",\
	"STATUS","UCSHELL","REMOTE","PRIORITY","DNS1","DNAT","SNAT","NETDATA","SESSION","ONCHANGED","RESTARTBASE","DISABLEPRN","ENABLEALLPRN","NTP",\
	"TIMEZONE","TZID","DYNAMIC_DNS","IP","VPN","GCOMPRESS","CONFIGSET","SAVE","RESTORE","NARI",\
    "END_OF_PATHFIELD"
}; //使用string类code及readwrite memory会增加很多

int GetEnumFromKeyword(char * key)
{
    int index;
    int start=0;
    int stop=CMD_END_OF_PATHFIELD;

    for(index = start; index < stop; index++)
    {
        if (strcmp(key, PATHTABLE[index]) == 0)
        {
            break;
        }
    }

    return index;
}
