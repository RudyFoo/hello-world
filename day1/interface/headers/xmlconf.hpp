/*
 * xmlconf.hpp
 *
 *  Created on: 2009-8-4
 *      Author: zekunyao
 */

#ifndef XMLCONF_HPP_
#define XMLCONF_HPP_

#include <string>
#include <pthread.h>
#include "tinyxml.h"
#include "aesFunc.h"
using namespace std;

#define 	MAX_NODE_DEPTH	(5)
#define     MEMORY_FILE_PATH 	"/tmp"
#define		CONFIG_PATH		"/usr/bin/geo/config/"

#define RET_OK				0x00
#define RET_INVALID_CMD		0x01
#define RET_INVALID_NODE	0x02
#define RET_INVALID_VALUE	0x03
#define RET_INVALID_OPERATION		0x04	//invalid operation
#define RET_ERR_PARAM		0x05	//Param number not correct
#define RET_EXEC_FAILED	0x06
#define RET_SYS_ERROR		0x07

#define XML_NUM   16
typedef enum {
	XML_DEVICE = 0,
    XML_SYSTEM=1,
	XML_NETWORK,
	XML_RECORD,
	XML_NTRIP,
	XML_PORTS,
	XML_USERS,
	XML_ANTENNALIST,
	XML_GPS,
	XML_DATAFLOW,
	XML_RADIO,
	XML_VPN,
	XML_NETDATA,
	XML_UNKNOWN       //11
}XML_ID;

typedef struct __XML{
    TiXmlDocument  doc;
    TiXmlDocument  template_doc;
    int changed;
}_XML;


class config
{
public:
	config();
	~config();
    int save_enable;
	int getValue(string path, string* value);
	int setValue(string path, string value);
	int getList(string path, string* value);
	int saveConfig();
	int saveRealtime();
    int getallValue(string &msg, TiXmlNode *node, string path);
    int getall(string &msg,int radio);
    int getallWithNode(string path, string &msg);
    int deleteNode(string path);
    int addNode(string path);
    int setNodeAttById(string path,int &n);
private:
	pthread_t thread;
    _XML xmls[XML_NUM];
	/*TiXmlDocument docNetwork;
	TiXmlDocument docDataflow;
	TiXmlDocument docDevice;
	TiXmlDocument docRecord;
	TiXmlDocument docGPS;
	//TiXmlDocument docGPSCard;
	TiXmlDocument docNtrip;
	TiXmlDocument docPorts;
	//TiXmlDocument docRegistration;
	//TiXmlDocument docSchedule;
	TiXmlDocument docServers;
	TiXmlDocument docSystem;
	TiXmlDocument docUsers;
	TiXmlDocument docAntennalist;
*/
	TiXmlDocument *getDocFromStr(string str,int &id);
	TiXmlElement* getElementFromPath(string path,string *att,int &id);
	TiXmlElement* creatElementFromPath(string path);
	TiXmlElement* getElementFromPathForSet(string path,int &id);
	string  getElemValue(TiXmlHandle handle, string *node,int depth,int &unknown_node);
	const char* getFullPath(char* fullpath, const char* filename);
	bool trySave(TiXmlDocument* doc);
	bool recoverXml(const unsigned char *date, int size,const char *filename);
	void encrypt(const char *xmlFile);
	static void *save_thread(void *);
};


#endif /* XMLCONF_HPP_ */
