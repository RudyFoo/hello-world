/*
 * share.hpp
 *
 *  Created on: 2009-8-3
 *      Author: zekunyao
 */

#ifndef SHARE_HPP_
#define SHARE_HPP_

#if   0//NOVATEL
#define _GPSBOARD_OEM_NOVATEL_
#else
#define _GPSBOARD_OEM_BD970_
#endif

#define APP_VERSION  "2.12(180807)"
#define WEB_VERSION  "2.12"

#define  DEV_INFO_ADDR  1024
#define REGI_ADDR  0x50

#define MAX_MSG_SIZE 1024
#define KEY_MSGQUEUE		0x8888
#define KEY_CMDMAILBOX	0x7777
#define SMEM_STATIC			"/tmp/bufstatic.dat"
#define SMEM_DIFF			"/tmp/bufdiff.dat"
#define SMEM_CONF			"/tmp/bufconf.dat"
#define SMEM_RECORD			"/tmp/record.dat"

#define SESSION_FILE_LOG_DIR  "/etc/record_session/"
#define PHP_INI "/opt/apache/conf/php.ini"

#define GPS_UNKNOWN     0
#define GPS_TRIMBLE          1
#define GPS_NOVATEL         2
#define GPS_UNICORECOMM  3
#define GPS_HEMISPHERE  4

#define PROC_MAIN			0x0001
#define PROC_INTERFACE		0x0002
#define PROC_NTRIP			0x0003
#define PROC_PANEL			0x0004
#define PROC_SERIAL			0x0005
#define PROC_DECODER		0x0006
#define PROC_RECORD			0x0007
#define PROC_CONFIG			0x0008
#define PROC_MSG			0x0009
#define PROC_HTTPD			0x000a
#define PROC_NETWORK		0x0011

#define MODE_LINK_1    0
#define MODE_LINK_2    1
#define MODE_LINK_6    2
#define MODE_LINK_7    3
#define MODE_LINK_8    4
#define MODE_LINK_9    5
#define MODE_LINK_10   6
#define MODE_LINK_GPS   12
#define MODE_LINK_UHF   13

//inter-process command fifo size
#define  IPC_CMD_SIZE8K 1

//#define  IPC_SERIAL_READ	  8001
//#define  IPC_SERIAL_WRITE	  8002
//gnss board response fifo,this is used for classify data to cmd response/NMEA/raw gnss data.
#define  IPC_GPS_RAW_READ	  8101
//default size, for novatel and trimble gnss board,use_raw_port=0;
#define  IPC_GPS_READ_SIZE8K 1
//bigger size for hemisphere,use_raw_port=1, record data from here
#define  IPC_GPS_RAW_READ_SIZE8K 16
//GPS board config cmd send FIFO, created by  cHemisphereCommand/cTrimbleCommand/...,used by oem/oem_novatel/..
#define  IPC_GPS_CONF_WRITE	  8102
#define  IPC_GPS_CONF_WRITE_SIZE8K 1

//#define  IPC_GPS_DIFF_READ	  8201
//#define  IPC_GPS_DIFF_WRITE	  8202

#define IPC_RECORD_SIZE8K  512
//#define  IPC_RECORD  8301
//ID-SIZE8K, OEM use this shared memory to record data
#define  RECORD_IPC_INFO      "8301-8"
//ID-SIZE8K, OEM use this shared memory to record data,for Novatel the data output frequency can be up to 100hz,then demand a lot of memory
#define  IPC_TCP_RECORD  8305
//ID-SIZE8K, OEM use this shared memory to record data,for Novatel the data output frequency can be up to 100hz,then demand a lot of memory
//#define  IPC_TCP_RECORD_INFO  "8305-512"
#define  IPC_TCP_RECORD_WRITE_INFO  "8306-1"
#define  IPC_NMEA_INFO  "8401-1"

#define IPC_TRANSPOND_CMD 3600
#define IPC_TRANSPOND_CMD_RESPONSE 3601

//#define IPC_TRANSPOND_CMD_READ_INFO "3600-1"
//#define IPC_TRANSPOND_CMD_WRITE_INFO "3601-1"


#ifdef  NSC200
#define  DEF_HTTP_PORT "80"
#define  HTTP_PORT_PATH "/opt/apache/conf/port.conf"
#define  RECORD_DISK "/geo/sd/"
#define  RECORD_PATH "/geo/sd/record/"
#define CLEAN_BIN_PATH "/geo/app/clean  \"/geo/sd/record/\""
#else
#define  HTTP_PORT_PATH "/opt/httpd2.4.2/conf/port.conf"
#define  RECORD_DISK "/media/mmcblk0/"
#define  RECORD_PATH "/media/mmcblk0/record/"
#define CLEAN_BIN_PATH "/usr/bin/geo/clean \"/media/mmcblk0/record/\""
#endif


#define OEM_BIN_PATH "/usr/bin/geo/oem"
#define OEM_NOVATEL_BIN_PATH "/usr/bin/geo/oem_novatel"
#define OEM_UNICORE_BIN_PATH "/usr/bin/geo/oem_unicore"
#define OEM_HEMISPHERE_BIN_PATH "/usr/bin/geo/oem_hemisphere"

#define OLED_BIN_PATH "/usr/bin/geo/OLED"
#define OLED2_BIN_PATH "/usr/bin/geo/OLED2"
#define NTRIP_BIN_PATH "/usr/bin/geo/ntrip"
//#define SERIAL_BIN_PATH "/usr/bin/geo/serial"
#define SERIAL2_BIN_PATH "/usr/bin/geo/serial2"

#define SOCKET_BIN_PATH "/usr/bin/geo/socket"
#define SOCKET_RAW_BIN_PATH "/usr/bin/geo/socket_raw"
#define SOCKET_REMOTE_BIN_PATH "/usr/bin/geo/socket_remote"
#define RECORD_BIN_PATH "/usr/bin/geo/record"
#define CMD_BIN_PATH "/usr/bin/geo/cmd"
#define NTRIP_CLIENT_BIN_PATH "/usr/bin/geo/ntrip_client"
#define NTRIP_CASTER_BIN_PATH "/usr/bin/geo/ntrip_caster"
#define TCP_SERVER_BIN_PATH "/usr/bin/geo/server"
#define HUB_BIN_PATH "/usr/bin/geo/hub"
#define BATCHUPDATE_BIN_PATH "/geo/app/batch_update"
#define REMOTEUPGRADE_BIN_PATH "/geo/app/remote_update"
#define RADIO_BIN_PATH "/geo/app/radio"
#define BINEX_BIN_PATH "/geo/app/toBinex"
#define RAW_TRANSPOND_BIN_PATH "/geo/app/raw_transpond"
#define MET_BIN_PATH "/usr/bin/geo/met-tilt"

#define GEO_PUSH "/geo/app/geopush"
#define FTP_PUSH_SCRIPT "/usr/bin/geo/scripts/ftppush.sh"
#define EMAIL_SCRIPT "/usr/bin/geo/scripts/sendmail.sh"
#define SMS_SCRIPT "/usr/bin/geo/scripts/sendsms.sh"

#define SELFTEST_SCRIPT "/usr/bin/geo/selftest BD970 /opt/log/selftest.log"

#define SETMAC_PATH "/sbin/ifconfig eth0 hw ether "


#define DIFF_SOCKET "C192.167.100.190:5017"
#define RECORD_SOCKET "C192.167.100.190:5018"

#define CMD_BT_ID 20
#define BT_SERIAL  "rfcomm0"
#define BT_SOCKET "C192.167.100.190:28002"
//#define BT_SOCKET_ID 23
//#define  IPC_BT_READ	  8701
//#define  IPC_BT_WRITE	  8702
//ID-SIZE8K
#define  IPC_BT_READ_INFO	  "8701-1"
//ID-SIZE8K
#define  IPC_BT_WRITE_INFO	  "8702-8"
#define  IPC_BT_WRITE_BINEX_INFO	  "8712-8"
//#define  IPC_CMD_READ	  IPC_BT_READ
//#define  IPC_CMD_WRITE	  IPC_BT_WRITE

#define CMD_COM_ID 21
#define CMD_COM2_ID 31
#define CMD_COM3_ID 41
#ifdef NSC200
#define COM_SERIAL   "ttyO4"
#define COM3_SERIAL   "ttyO0"
#else
#define COM_SERIAL   "ttyUSB/cp210x1"//"ttyUSB3"
#endif

#define COM_SOCKET "C192.167.100.190:28001"
#define COM_SOCKET_ID 22
//#define  IPC_COM_READ	  8801
//#define  IPC_COM_WRITE	  8802
#define  IPC_COM_READ_INFO	  "8801-1"
#define  IPC_COM_WRITE_INFO	  "8802-8"
//todo:check the two fifo id is write -- IPC_COM_WRITE_INFO,IPC_COM_WRITE_BINEX_INFO
#define  IPC_COM_WRITE_BINEX_INFO	  "8807-8"
//#define  IPC_COM2_READ    8811
//#define  IPC_COM2_WRITE   8812
#define  IPC_COM2_READ_INFO    "8811-1"
#define  IPC_COM2_WRITE_INFO   "8812-8"
//todo:check the two fifo id is write -- IPC_COM2_WRITE_INFO,IPC_COM2_WRITE_BINEX_INFO
#define  IPC_COM2_WRITE_BINEX_INFO   "8817-8"

#define  IPC_COM3_READ_INFO    "8821-1"
#define  IPC_COM3_WRITE_INFO   "8822-8"
//todo:check the two fifo id is write -- IPC_COM3_WRITE_INFO,IPC_COM3_WRITE_BINEX_INFO
#define  IPC_COM3_WRITE_BINEX_INFO   "8827-8"

#define  IPC_MET_WRITE_INFO     "8405-1"

#define CMD_NTRIP_CLIENT_ID 22
#define NTRIP_CLIENT_SOCKET "C192.167.100.190:5017"
#define NTRIP_CLIENT_SOCKET_ID 20
//#define  IPC_NTRIP_CLIENT_READ	  8905
//#define  IPC_NTRIP_CLIENT_WRITE	  8906
#define  IPC_NTRIP_CLIENT_READ_INFO	  "8905-1"
#define  IPC_NTRIP_CLIENT_WRITE_INFO	  "8906-1"

#define TCP_DEFAULT_SIZE8K 1
#define TCP_CMD_SIZE8K 8
#define TCP_GNSSRAW_SIZE8K  128
#define TCP_GNSSNMEA_SIZE8K  128
#define TCP_GNSSRTKOUT_SIZE8K  2
#define TCP_GNSSBINEXOUT_SIZE8K  128

#define CMD_TCP_ID 23
#define TCP_SOCKET "C192.167.100.190:5020"
#define TCP_SOCKET_ID 29
//#define  IPC_TCP_READ	  8901
#define  IPC_TCP_READ_INFO	  "8901-1"
//GNSS data
#define  IPC_TCP_WRITE	  8902
#define  IPC_TCP_WRITE_BINEX	  8907
//#define  IPC_TCP_WRITE_INFO	  "8902-1"
//#define  IPC_TCP_WRITE_BINEX_INFO	  "8907-1"

#define CMD_TCP1_ID 24
#define TCP1_SOCKET "C192.167.100.190:5021"
#define TCP1_SOCKET_ID 28
//#define  IPC_TCP1_READ	  8911
#define  IPC_TCP1_READ_INFO	  "8911-1"
#define  IPC_TCP1_WRITE	  8912
#define  IPC_TCP1_WRITE_BINEX	  8917
//#define  IPC_TCP1_WRITE_INFO	  "8912-1"
//#define  IPC_TCP1_WRITE_BINEX_INFO	  "8917-1"


#define CMD_TCP2_ID 25
#define TCP2_SOCKET "C192.167.100.190:5022"
#define TCP2_SOCKET_ID 27
//#define  IPC_TCP2_READ	  8921
#define  IPC_TCP2_READ_INFO	  "8921-1"
#define  IPC_TCP2_WRITE	  8922
#define  IPC_TCP2_WRITE_BINEX	  8927
//#define  IPC_TCP2_WRITE_INFO	  "8922-1"
//#define  IPC_TCP2_WRITE_BINEX_INFO	  "8927-1"


#define NTRIP_SOCKET_IP "C192.167.100.190"
#define NTRIP_SOCKET_PORT  5023
#define NTRIP_SOCKET_ID 26
#define  IPC_NTRIP_SOCKET_READ	  5001
#define  IPC_NTRIP_SOCKET_READ_SIZE8K	  1
#define  IPC_NTRIP_SOCKET_WRITE	  (IPC_NTRIP_SOCKET_READ+1)
#define  IPC_NTRIP_SOCKET_WRITE_SIZE8K	  1

#define  RADIO_SERIAL  "ttyUSB/cp210x1"
#define  RADIO_SERIAL_V4  "ttyUSB/cp210x0"
//#define  IPC_RADIO_READ     	  (IPC_GPS_DIFF_WRITE)
//#define  IPC_RADIO_READ_SIZE8K   	  1
//#define  IPC_RADIO_WRITE   	  (IPC_GPS_DIFF_READ)
//#define  IPC_RADIO_WRITE_SIZE8K   	  1
#define  IPC_RADIO_READ_INFO     	  "8202-1"
#define  IPC_RADIO_WRITE_INFO   	  "8201-1"
#define  IPC_RADIO_WRITE_BINEX_INFO   	  "8206-1"


#define  IPC_RADIO_CMD_RESPONSE     	  (7751)
//#define  IPC_RADIO_CMD_READ_SIZE8K   	  1
#define  IPC_RADIO_CMD   	  (7752)
//#define  IPC_RADIO_CMD_WRITE_SIZE8K   	  1


//#define  IPC_REMOTE_READ  5083
//#define  IPC_REMOTE_WRITE  5084
#define  IPC_REMOTE_READ_INFO  "5083-128"
#define  IPC_REMOTE_WRITE_INFO  "5084-1"

#define  CMD_REMOTE_ID  35

//#define   IPC_DIFF2_READ   8203
//#define   IPC_DIFF2_WRITE   8204

#define STR_STATUS_NO_FREE_EP "error: no free endpoint"
#define STR_STATUS_SET_INTERVAL_ERR "idle: set output interval error"
#define STR_STATUS_USB_CONNECTED "error: USB connected"

#define SEM_STRING			"cmdmailbox"

#define MSG_LEN				(2048)
#define MEM_BUFF_LEN		(8192)

#define CheckRegi   1

typedef struct msgmbuf
{
 long msg_type;
 long msg_source;
 char msg_text[MSG_LEN-8];
} MSG_BUFF;

typedef struct msgmail
{
 long msg_type;
 long flag;
 long flag2;
} MSG_MAILBOX;

typedef struct membuf
{
 int length;
 char msg_text[MEM_BUFF_LEN-4];
} MEM_BUFF;

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

#define MAX_PATH 1024
#endif /* SHARE_HPP_ */

