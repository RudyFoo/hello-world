/*
 * xmlconf.cpp
 *
 *  Created on: 2009-8-3
 *      Author: zekunyao
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <syslog.h>
#include     <fcntl.h>
#include <unistd.h>
#include "tinyxml.h"
#include "xmlconf.hpp"
#include "util.hpp"
using namespace std;

#include "data.c"

pthread_mutex_t mutex_save;
pthread_cond_t cond_save;

bool config::recoverXml(const unsigned char *date, int size,const char *filename)
{
    char path[128];
    int file;
    memset(path,0,sizeof(path));
    strcpy(path, CONFIG_PATH);
	strcat(path, filename);

	syslog(LOG_LOCAL7|LOG_WARNING,"recover xml file %s",filename);

#ifdef ENCRYPT_XML
    char pathtmp[128];
	memset(pathtmp,0,sizeof(pathtmp));
    strcpy(pathtmp, CONFIG_PATH);
    strcpy(pathtmp, ".");
	strcat(pathtmp, filename);

	file = open(pathtmp, O_RDWR|O_CREAT|O_TRUNC ,0666);
	if (file < 0) {
        printf(" Unable to open file!");
        return false;
	}
	write(file, date,size);
    close(file);
    fsync(file);

    AES_Encrypt(pathtmp,path,HASH_SHA256);
    remove(pathtmp);
#else
	file = open(path, O_RDWR|O_CREAT|O_TRUNC ,0666);
	if (file < 0) {
        printf(" Unable to open file!");
        return false;
	}
	write(file, date,size);
    close(file);
    fsync(file);
#endif
    return true;
}
config::config()
{
	int result = 0;
	save_enable = true;
	//char errStr[64];
	char path[128];
	int i;
	for(i=0;i<XML_NUM;i++)
	{
	    xmls[i].changed=0;
	}

	//SetCondenseWhiteSpace(false);
	if(access("/etc/encrypt_xml",F_OK|R_OK)!=0)
    {
        syslog(LOG_LOCAL7|LOG_WARNING,"Config files will be encrypted !!!");
        system("touch /etc/encrypt_xml");

        encrypt("network.xml");
        encrypt("device.xml");
        encrypt("record.xml");
        encrypt("ntrip.xml");
        encrypt("ports.xml");
        encrypt("radio.xml");
        encrypt("system.xml");
        encrypt("users.xml");
        encrypt("vpn.xml");
    }

	if (xmls[XML_NETWORK].doc.LoadFile(getFullPath(path,"network.xml")) == false)
	{
	    recoverXml(network_xml, sizeof(network_xml), (const char *)"network.xml");
	    if (xmls[XML_NETWORK].doc.LoadFile(getFullPath(path,"network.xml")) == false)
	    {
            result = 1;
	    }
	}
    xmls[XML_NETWORK].template_doc.Parse((const char *)network_xml,0,TIXML_DEFAULT_ENCODING);
    //xmls[XML_NETWORK].template_doc.Print();

	if (xmls[XML_DATAFLOW].doc.LoadFile(getFullPath(path,"dataflow.xml")) == false)
    {
	    recoverXml(dataflow_xml, sizeof(dataflow_xml), (const char *)"dataflow.xml");
	    if (xmls[XML_DATAFLOW].doc.LoadFile(getFullPath(path,"dataflow.xml")) == false)
	    {
            result = 2;
	    }
	}
	xmls[XML_DATAFLOW].template_doc.Parse((const char *)dataflow_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_DEVICE].doc.LoadFile(getFullPath(path,"device.xml")) == false)
    {
	    recoverXml(device_xml, sizeof(device_xml), (const char *)"device.xml");
	    if (xmls[XML_DEVICE].doc.LoadFile(getFullPath(path,"device.xml")) == false)
	    {
            result = 3;
	    }
	}
	xmls[XML_DEVICE].template_doc.Parse((const char *)device_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_RECORD].doc.LoadFile(getFullPath(path,"record.xml")) == false)
    {
	    recoverXml(record_xml, sizeof(record_xml), (const char *)"record.xml");
	    if (xmls[XML_RECORD].doc.LoadFile(getFullPath(path,"record.xml")) == false)
	    {
            result =4 ;
	    }
	}
	xmls[XML_RECORD].template_doc.Parse((const char *)record_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_GPS].doc.LoadFile(getFullPath(path,"gps.xml")) == false)
    {
	    recoverXml(gps_xml, sizeof(gps_xml), (const char *)"gps.xml");
	    if (xmls[XML_GPS].doc.LoadFile(getFullPath(path,"gps.xml")) == false)
	    {
            result =5 ;
	    }
	}
	xmls[XML_GPS].template_doc.Parse((const char *)gps_xml,0,TIXML_DEFAULT_ENCODING);
/*
	if (docGPSCard.LoadFile(getFullPath(path,"gpscard.xml")) == false)
	{
	    recoverXml(gpscard_xml, sizeof(gpscard_xml), (const char *)"gpscard.xml");
	    if (docNetwork.LoadFile(getFullPath(path,"gpscard.xml")) == false)
	    {
            result =6 ;
	    }
	}
*/
    if (xmls[XML_RADIO].doc.LoadFile(getFullPath(path,"radio.xml")) == false)
    {
	    recoverXml(radio_xml, sizeof(radio_xml), (const char *)"radio.xml");
	    if (xmls[XML_RADIO].doc.LoadFile(getFullPath(path,"radio.xml")) == false)
	    {
            result =6 ;
	    }
	}
	xmls[XML_RADIO].template_doc.Parse((const char *)radio_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_NTRIP].doc.LoadFile(getFullPath(path,"ntrip.xml")) == false)
	{
	    recoverXml(ntrip_xml, sizeof(ntrip_xml), (const char *)"ntrip.xml");
	    if (xmls[XML_NTRIP].doc.LoadFile(getFullPath(path,"ntrip.xml")) == false)
	    {
            result =7 ;
	    }
	}
	xmls[XML_NTRIP].template_doc.Parse((const char *)ntrip_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_PORTS].doc.LoadFile(getFullPath(path,"ports.xml")) == false)
	{
	    recoverXml(ports_xml, sizeof(ports_xml), (const char *)"ports.xml");
	    if (xmls[XML_PORTS].doc.LoadFile(getFullPath(path,"ports.xml")) == false)
	    {
            result =8;
	    }
	}
	xmls[XML_PORTS].template_doc.Parse((const char *)ports_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_NETDATA].doc.LoadFile(getFullPath(path,"netData.xml")) == false)
    {
	    recoverXml(netData_xml, sizeof(netData_xml), (const char *)"netData.xml");
	    if (xmls[XML_NETDATA].doc.LoadFile(getFullPath(path,"netData.xml")) == false)
	    {
            result =9 ;
	    }
	}
	xmls[XML_NETDATA].template_doc.Parse((const char *)netData_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_VPN].doc.LoadFile(getFullPath(path,"vpn.xml")) == false)
	{
	    recoverXml(vpn_xml, sizeof(vpn_xml), (const char *)"vpn.xml");
	    if (xmls[XML_VPN].doc.LoadFile(getFullPath(path,"vpn.xml")) == false)
	    {
            result =10;
	    }
	}
	xmls[XML_VPN].template_doc.Parse((const char *)vpn_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_SYSTEM].doc.LoadFile(getFullPath(path,"system.xml")) == false)
	{
	    recoverXml(system_xml, sizeof(system_xml), (const char *)"system.xml");
	    if (xmls[XML_SYSTEM].doc.LoadFile(getFullPath(path,"system.xml")) == false)
	    {
            result =12 ;
	    }
	}
	xmls[XML_SYSTEM].template_doc.Parse((const char *)system_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_USERS].doc.LoadFile(getFullPath(path,"users.xml")) == false)
	{
	    recoverXml(users_xml, sizeof(users_xml), (const char *)"users.xml");
	    if (xmls[XML_USERS].doc.LoadFile(getFullPath(path,"users.xml")) == false)
	    {
            result =13 ;
	    }
	}
	xmls[XML_USERS].template_doc.Parse((const char *)users_xml,0,TIXML_DEFAULT_ENCODING);

	if (xmls[XML_ANTENNALIST].doc.LoadFile(getFullPath(path,"antennalist.xml")) == false)
	{
	    recoverXml(antennalist_xml, sizeof(antennalist_xml), (const char *)"antennalist.xml");
	    if (xmls[XML_ANTENNALIST].doc.LoadFile(getFullPath(path,"antennalist.xml")) == false)
	    {
            result =14 ;
	    }
	}
	xmls[XML_ANTENNALIST].template_doc.Parse((const char *)antennalist_xml,0,TIXML_DEFAULT_ENCODING);

	if (result > 0 ) {
		//sprintf(errStr, "Load xml files error %d", result);
		//perror(errStr);
		syslog(LOG_LOCAL7|LOG_ERR,"Load xml files error %d", result);
		exit(1);
	}
	else
	{
		xmls[XML_GPS].doc.SaveFile("/tmp/gps.xml");
		xmls[XML_DATAFLOW].doc.SaveFile("/tmp/dataflow.xml");
		xmls[XML_NETDATA].doc.SaveFile("/tmp/netData.xml");
	}

	pthread_mutex_init(&mutex_save,NULL);
	pthread_cond_init(&cond_save,NULL);

	if(pthread_create(&thread, NULL, save_thread, (void *)this)!=0)
	{
		syslog(LOG_LOCAL7|LOG_ERR,"save_thread create error");
		//exit(3);
	}
}

config::~config()
{

}

void *config::save_thread(void *p)
{
	pthread_detach(pthread_self());
	bool result = true;

	config *xmlconfig = (config *)p;

	while(1)
	{
		pthread_cond_wait(&cond_save,&mutex_save);

		if(xmlconfig->xmls[XML_NETWORK].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_NETWORK].doc)) result = false;
			xmlconfig->xmls[XML_NETWORK].changed=0;
		}
		if(xmlconfig->xmls[XML_DEVICE].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_DEVICE].doc)) result = false;
			xmlconfig->xmls[XML_DEVICE].changed=0;
		}
		if(xmlconfig->xmls[XML_RECORD].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_RECORD].doc)) result = false;
			xmlconfig->xmls[XML_RECORD].changed=0;
		}
		if(xmlconfig->xmls[XML_NTRIP].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_NTRIP].doc)) result = false;
			xmlconfig->xmls[XML_NTRIP].changed=0;
		}
		if(xmlconfig->xmls[XML_PORTS].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_PORTS].doc)) result = false;
			xmlconfig->xmls[XML_PORTS].changed=0;
		}
		if(xmlconfig->xmls[XML_SYSTEM].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_SYSTEM].doc)) result = false;
			xmlconfig->xmls[XML_SYSTEM].changed=0;
		}
		if(xmlconfig->xmls[XML_USERS].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_USERS].doc)) result = false;
			xmlconfig->xmls[XML_USERS].changed=0;
		}
		if(xmlconfig->xmls[XML_RADIO].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_RADIO].doc)) result = false;
			xmlconfig->xmls[XML_RADIO].changed=0;
		}
		if(xmlconfig->xmls[XML_VPN].changed)
		{
			if (!xmlconfig->trySave(&xmlconfig->xmls[XML_VPN].doc)) result = false;
			xmlconfig->xmls[XML_VPN].changed=0;
		}

		sync();

		pthread_mutex_unlock(&mutex_save);
	}

	pthread_exit(0);
	return 0;
}

string config::getElemValue(TiXmlHandle handle, string *node,int depth ,int &unknown_node)
{
    string value,text;
    string tempStr,file,nodeStr,attrName,attrStr;
    int split;
	TiXmlElement* xmlnode;
	TiXmlElement* elem ;
	const char *c;
	int index = 0;
	int flag=1;
	unknown_node=0;
	value.clear();
	attrName.clear();
	attrStr.clear();
	//printf("\n\nindex=%d, depth=%d\n",index,depth);
	//for(int i=0;i<depth;i++) printf("%d %s\n",i,node[i].c_str());
    while (index<depth && flag)
	{
		tempStr = node[index];
		if ((split = tempStr.find('@',0)) > 0)
		{
			//printf("Split %s at %d",tempStr.c_str(),split);
			nodeStr = tempStr.substr(0,split);
			attrName = tempStr.substr(split+1,tempStr.length());
			if ((split = attrName.find(':',0)) > 0)
			{
			    attrStr = attrName.substr(split+1,attrName.length());
			    attrName = attrName.substr(0,split);
			}
			else
			{

			}
			transform(nodeStr.begin(), nodeStr.end(), nodeStr.begin(), ::toupper);
			transform(attrName.begin(), attrName.end(), attrName.begin(), ::toupper);
			//cout << "Node: " << nodeStr << " Name: " << attrName << " Attr: " << attrStr << endl;
			handle = handle.FirstChild(nodeStr.c_str());
			xmlnode = handle.Element();
			//if((xmlnode == NULL)) cout << "null" << endl;
			value.assign("");
			unknown_node=1;
			while (xmlnode != NULL)
			{
				if (xmlnode->Attribute(attrName.c_str()) != NULL)
				{
				    if (attrStr.compare("*") == 0 || attrStr.compare(xmlnode->Attribute(attrName.c_str())) == 0)
				    {
						//cout << xmlnode->Attribute(attrName.c_str())<<  endl;
						handle = TiXmlHandle(xmlnode);
						unknown_node=0;
						//cout<< "node "<<xmlnode->Value()<<endl;
						if(index<(depth-1))
                         {
                             value+=getElemValue(handle,&node[index+1],depth-index-1,unknown_node)+"|";
                             flag=0;
                         }
						 else
						 {
						       elem = handle.Element();
                                if (elem == NULL)
                                {
                                    //cout<< "unknown node"<<endl;
                                    unknown_node=1;
                                    return value;
                                }

                                c=elem->GetText();
                                if(c!=NULL)
                                    text .assign( c);
                                //cout<< "text "<<text<<endl;
                                if (text.empty())
                                {
                                    text.assign("");
                                }
                                value+=text+"|";
                                //xmlnode = NULL;
                                //break;
						 }
                    }
                    xmlnode = xmlnode->NextSiblingElement(nodeStr.c_str());
				}
				else xmlnode=NULL;
			}
		}
		else
		{
		    transform(tempStr.begin(), tempStr.end(), tempStr.begin(), ::toupper);
			handle = handle.FirstChild(tempStr.c_str());
			//cout << "Session: |" << tempStr << "| value "<< endl;
			if(index<(depth-1))
			{

			}
			else
			{
			    elem = handle.Element();
                if (elem == NULL)
                {
                    //cout<< "unknown node2"<<endl;
                    unknown_node=1;
                    return value;
                }
                c=elem->GetText();
                if(c!=NULL)
                    text .assign( c);
                //cout<< "text "<<text<<endl;
                if (text.empty())
                {
                    text.assign("");
                }
                value+=text+"|";
			}
		}
		index++;
		//xmlnode = handle.Element();
		//cout << xmlnode->Value() << endl;
	}

    //cout<< "value "<<value<<endl;
    if(*(value.c_str()+value.length()-1)=='|')  value = value.substr(0,value.length()-1);
	return value;
}

int config::getValue(string path, string* value)
{

	string file;
	string node[MAX_NODE_DEPTH];
	unsigned int index,depth,start,end;
	//const char* text ;
	index = 0;
	depth = 0;
	//cout << "Receive string:" << path << endl;
	while ((index < path.length()) && (depth < MAX_NODE_DEPTH))
	{
		start = index;
		while ((index < path.length()) && (path[index] != '\\')&& (path[index] != '.'))
			index++;
		end = index;

		if (depth == 0)
		{
			file = path.substr(start,end - start);
			//cout << "filename: " << file <<endl;
		}
		node[depth] = path.substr(start,end - start);
		//transform(node[depth] .begin(), node[depth] .end(), node[depth] .begin(), ::toupper);
		//printf("Session %d-%d index = %d depth=%d \r\n",start,end,index,depth);
		depth++;
		index++;
	}
	//sleep(5);
	int id;
	int error;
	TiXmlDocument *doc = getDocFromStr(file,id);
	if (doc == NULL)
	{
		cout << "Unable to find document.\r\n";
		return RET_INVALID_NODE;
	}
	TiXmlHandle handle(doc);
	string tempStr;
	tempStr = node[0];
	transform(tempStr.begin(), tempStr.end(), tempStr.begin(), ::toupper);
    handle = handle.FirstChild(tempStr.c_str());
    value->assign(getElemValue(handle,&node[1],depth-1,error));
	//cout << index << "AA" << depth ;
    if(error)
    {
        value->clear();
        return RET_INVALID_NODE;
    }
	else  return RET_OK;
}

int config::getList(string path, string* value)
{
    int id;
    string node,att;
    att.assign("");
    value->assign("");
	TiXmlElement* xmlnode = getElementFromPath(path,&att,id);

	string text;
	if (xmlnode == NULL || att.length()==0)
		return RET_INVALID_NODE;

	if (xmlnode->Attribute(att.c_str()) != NULL )
		text.assign(xmlnode->Attribute(att.c_str())) ;
	else
		return RET_INVALID_NODE;

    node.assign(xmlnode->Value());
	//cout<< text<<endl;
	while (!text.empty())
	{
		//value->append(xmlnode->Value());
		//value->append("," + text);
		//value->append("\n");
		//cout<< xmlnode->Value()<<endl;
		value->append(text);
		xmlnode = xmlnode->NextSiblingElement(node.c_str());
		if (xmlnode == NULL)
		{
			text.clear();
		}
		else
		{
		    text.clear();
		    if (xmlnode->Attribute(att.c_str()) != NULL )
		    {
		        text.assign(xmlnode->Attribute(att.c_str()));
			    value->append("|") ;
		    }
		}
	}

	return RET_OK;
}

int config::setValue(string path, string value)
{
	//cout << "Value: " << value << endl;
	//cout << "path: " << path << endl;
	int id;
	TiXmlElement* xmlnode = getElementFromPathForSet(path,id);

	if (xmlnode == NULL)
	{
	    return RET_INVALID_NODE;
	}


	if (xmlnode->FirstChild() == NULL)
		xmlnode->LinkEndChild(new TiXmlText(value.c_str()));
	else
	{
	    if(xmlnode->FirstChild()->ToText()==NULL)
	    {
	        return RET_INVALID_NODE;
	    }
        xmlnode->FirstChild()->ToText()->SetValue(value.c_str());
	}


    xmls[id].changed=1;
    //printf("id %d\n",id);
	return RET_OK;
}

int config::deleteNode(string path)
{
	//cout << "Value: " << value << endl;
	//cout << "path: " << path << endl;
	int id;
	TiXmlElement* xmlelem= getElementFromPathForSet(path,id);
	if (xmlelem == NULL)
	{
	    return RET_INVALID_NODE;
	}

    TiXmlElement * xmlparent;

    xmlparent=xmlelem->Parent()->ToElement();
    if(xmlparent==NULL)
    {
	    return RET_INVALID_NODE;
	}

    if(!xmlparent->RemoveChild(xmlelem))
    {
	    return RET_EXEC_FAILED;
	}

    xmls[id].changed=1;
    //printf("id %d\n",id);
	return RET_OK;
}

int config::addNode(string path)
{
	//cout << "Value: " << value << endl;
	//cout << "path: " << path << endl;
    string value;
    if(getValue(path,&value)==RET_OK)
    {
        return RET_OK;
    }

    if(creatElementFromPath(path)!=NULL)
        return RET_OK;
    else
        return RET_EXEC_FAILED;
}

int config::setNodeAttById(string path,int &n)
{
    int id;
    int count=0;
    char str[32];
    string att;
    n=0;
	TiXmlElement* xmlnode = getElementFromPath(path,&att,id);

	string node,text;
	if (xmlnode == NULL )
		return RET_INVALID_NODE;

    node.assign(xmlnode->Value());
	//cout<< text<<endl;
	while (xmlnode != NULL)
	{
	    if(xmlnode->Attribute("ID")!=NULL)
	    {
            sprintf(str,"%d",count);
            xmlnode->SetAttribute("ID",str);
	    }
        count++;
		xmlnode = xmlnode->NextSiblingElement(node.c_str());
	}
    n=count;
    return RET_OK;


}
int config::getallValue(string &msg, TiXmlNode *node, string path)
{
    char tmp[32];
    string str;
    string line;
    TiXmlAttribute *attr;

    if(node)
    {
        TiXmlNode*    elem = node->FirstChild();
        for (; elem; elem=elem->NextSibling())
        {
             int Type = elem->Type();
             switch ( Type )
             {
                  case TiXmlNode::TINYXML_ELEMENT:
                        //printf("%s.",elem->Value());
                        str.assign(elem->Value());
                        attr=elem->ToElement()->FirstAttribute();
                        if(attr!=NULL)
                        {
                            memset(tmp,0,sizeof(tmp));
                            sprintf(tmp,"@%s:%s",attr->Name(),attr->Value());
                            str = str + tmp;
                        }
                        //path+=str+".";
                        getallValue(msg,elem, path+"."+str);
                        break;
                  case TiXmlNode::TINYXML_TEXT:
                        //printf("[%s]\n",elem->Value());
                        //printf("%s,OK,%s\n",path.c_str(), elem->Value());
                        str.assign(elem->Value());
                        line="@GNSS,"+path+",OK,"+str;
                        sprintf(tmp,"*%02X\r\n",cmd_crc8((unsigned char   *)line.c_str(),line.length()));
                        line.append(tmp);
                        msg.append(line);
                        break;
                   default: break;
             }
        }
    }
    return RET_OK;
}

int config::getallWithNode(string path, string &msg)
{
    int id;
    unsigned int i;
    string nodestr;
	string file;
	string node[MAX_NODE_DEPTH];
	unsigned int index,depth,start,end;
	//const char* text ;
	index = 0;
	depth = 0;
	//cout << "Receive string:" << path << endl;
	while ((index < path.length()) && (depth < MAX_NODE_DEPTH))
	{
		start = index;
		while ((index < path.length()) && (path[index] != '\\')&& (path[index] != '.'))
			index++;
		end = index;

		if (depth == 0)
		{
			file = path.substr(start,end - start);
			//cout << "filename: " << file <<endl;
		}
		node[depth] = path.substr(start,end - start);
		transform(node[depth] .begin(), node[depth] .end(), node[depth] .begin(), ::toupper);
		//printf("Session depth=%d  %s\r\n",depth,node[depth].c_str());
		depth++;
		index++;
	}

	TiXmlDocument *doc = getDocFromStr(file,id);
	if (doc == NULL) return RET_INVALID_NODE;

	msg.clear();
	transform(file.begin(), file.end(), file.begin(), ::toupper);
	TiXmlNode *xmlnode=doc->FirstChild(file.c_str());
	if (xmlnode == NULL) return RET_INVALID_NODE;

    nodestr="GET,"+file;
    for(i=1;i<depth;i++)
    {
        xmlnode=xmlnode->FirstChild(node[i] .c_str());
        if (xmlnode == NULL) return RET_INVALID_NODE;
        nodestr=nodestr+"."+node[i];
    }

	return getallValue(msg,xmlnode,nodestr);
}

int config::getall(string &msg,int radio)
{
    //docDevice.Print();
    //TiXmlHandle handle(docDevice);
    msg.clear();
    getallValue(msg,xmls[XML_DEVICE].doc.FirstChild("DEVICE"),"GET,DEVICE");
    getallValue(msg,xmls[XML_SYSTEM].doc.FirstChild("SYSTEM"),"GET,SYSTEM");
    getallValue(msg,xmls[XML_NETWORK].doc.FirstChild("NETWORK"),"GET,NETWORK");
    getallValue(msg,xmls[XML_RECORD].doc.FirstChild("RECORD"),"GET,RECORD");
    getallValue(msg,xmls[XML_NTRIP].doc.FirstChild("NTRIP"),"GET,NTRIP");
    getallValue(msg,xmls[XML_PORTS].doc.FirstChild("PORTS"),"GET,PORTS");
    getallValue(msg,xmls[XML_GPS].doc.FirstChild("GPS"),"GET,GPS");
    getallValue(msg,xmls[XML_DATAFLOW].doc.FirstChild("DATAFLOW"),"GET,DATAFLOW");
    if(radio) getallValue(msg,xmls[XML_RADIO].doc.FirstChild("RADIO"),"GET,RADIO");
    return RET_OK;
}

int config::saveConfig()
{
	bool result = true;
	//char path[128];
	//if (!docDataflow.SaveFile(getFullPath(path,"dataflow.xml")))
	//result = false;
	//docDataflow.SaveFile("/tmp/dataflow.xml");

	//save a copy in temp file for web display;
/*	docDevice.SaveFile();
	docFileList.SaveFile();
	//docGPSCard.SaveFile();
	//gps card information shouldn't change
	docNtrip.SaveFile();
	docPorts.SaveFile();
	docRegistration.SaveFile();
	docSchedule.SaveFile();
	docServers.SaveFile();
	docSystem.SaveFile();
	docUsers.SaveFile();
	//Antennalist shouldn't change
*/
    if(save_enable == false)
        return RET_OK;

	pthread_cond_signal(&cond_save);

	return RET_OK;
/*
    if(xmls[XML_NETWORK].changed)
    {
        if (!trySave(&xmls[XML_NETWORK].doc)) result = false;
        xmls[XML_NETWORK].changed=0;
    }
	if(xmls[XML_DEVICE].changed)
    {
        if (!trySave(&xmls[XML_DEVICE].doc)) result = false;
        xmls[XML_DEVICE].changed=0;
    }
    if(xmls[XML_RECORD].changed)
    {
        if (!trySave(&xmls[XML_RECORD].doc)) result = false;
        xmls[XML_RECORD].changed=0;
    }
    if(xmls[XML_NTRIP].changed)
    {
        if (!trySave(&xmls[XML_NTRIP].doc)) result = false;
        xmls[XML_NTRIP].changed=0;
    }
    if(xmls[XML_PORTS].changed)
    {
        if (!trySave(&xmls[XML_PORTS].doc)) result = false;
        xmls[XML_PORTS].changed=0;
    }
    if(xmls[XML_SYSTEM].changed)
    {
        if (!trySave(&xmls[XML_SYSTEM].doc)) result = false;
        xmls[XML_SYSTEM].changed=0;
    }
    if(xmls[XML_USERS].changed)
    {
        if (!trySave(&xmls[XML_USERS].doc)) result = false;
        xmls[XML_USERS].changed=0;
    }
    if(xmls[XML_RADIO].changed)
    {
        if (!trySave(&xmls[XML_RADIO].doc)) result = false;
        xmls[XML_RADIO].changed=0;
    }
	if (result)
	{
	    sync();
	    return RET_OK;
	}

	else
		return RET_SYS_ERROR;*/
}

bool config::trySave(TiXmlDocument* doc)
{
	if (doc->SaveFile())
	{
		return true;
	}
	else
	{
		printf("Writing %s error\n", doc->Value());
		return false;
	}
}

int config::saveRealtime()
{
	//bool result;
	if (xmls[XML_DATAFLOW].doc.SaveFile("/tmp/dataflow.xml") == false)
		cout << "Write temperary file error." << endl;
	if (xmls[XML_GPS].doc.SaveFile("/tmp/gps.xml") == false)
		cout << "Write temperary file error." << endl;
    if (xmls[XML_NETDATA].doc.SaveFile("/tmp/netData.xml") == false)
		cout << "Write temperary file error." << endl;
	return RET_OK;
}


TiXmlElement* config::getElementFromPath(string path,string *att,int &id)
{
	string tempStr,file,nodeStr,attrName,attrStr;
	string node[MAX_NODE_DEPTH];
	unsigned int index,depth,start,end;
	int split;
	bool found;
	index = 0;
	depth = 0;
	//cout << "Receive string:" << path << endl;
	while ((index < path.length()) && (depth < MAX_NODE_DEPTH))
	{
		start = index;
		while ((index < path.length()) && (path[index] != '\\')&& (path[index] != '.'))
			index++;
		end = index;

		if (depth == 0)
		{
			file = path.substr(start,end - start);
			//cout << "filename: " << file <<endl;
		}
		node[depth] = path.substr(start,end - start);
		//transform(node[depth] .begin(), node[depth] .end(), node[depth] .begin(), ::toupper);
		//printf("Session %d-%d index = %d depth=%d \r\n",start,end,index,depth);
		depth++;
		index++;
	}
	//sleep(5);
	TiXmlDocument *doc = getDocFromStr(file,id);
	if (doc == NULL)
	{
		cout << "Unable to find document.\r\n";
		return NULL;
	}
	TiXmlHandle handle(doc);
	TiXmlElement* xmlnode;
	index = 0;
    attrStr="";attrName="";
	//cout << index << "AA" << depth ;
	while (index<depth)
	{
		tempStr = node[index];
		if ((split = tempStr.find('@',0)) > 0)
		{
			found = false;
			//printf("Split %s at %d",tempStr.c_str(),split);
			nodeStr = tempStr.substr(0,split);
			attrName = tempStr.substr(split+1,tempStr.length());

			if ((split = attrName.find(':',0)) > 0)
			{
			    attrStr = attrName.substr(split+1,attrName.length());
			    attrName = attrName.substr(0,split);
			}

			//cout << "Node: " << nodeStr << " Attr: " << attrName << " AttrStr: " << attrStr << endl;
			transform(nodeStr.begin(), nodeStr.end(), nodeStr.begin(), ::toupper);
			transform(attrName.begin(), attrName.end(), attrName.begin(), ::toupper);
            att->assign(attrName.c_str());

			handle = handle.FirstChild(nodeStr.c_str());
			xmlnode = handle.Element();
			while (xmlnode != NULL)
			{
				if (xmlnode->Attribute(attrName.c_str())!= NULL)
					if (attrStr.compare(xmlnode->Attribute(attrName.c_str())) == 0)
						found = true;
				if (found)
				{
					//index++;
					handle = TiXmlHandle(xmlnode);
					xmlnode = NULL;
					//cout << "Found matching" << endl;
				}
				else
					xmlnode = xmlnode->NextSiblingElement(nodeStr.c_str());
			}
		}
		else
		{
		    transform(tempStr.begin(), tempStr.end(), tempStr.begin(), ::toupper);
			handle = handle.FirstChild(tempStr.c_str());
			//cout << "Session: |" << tempStr << "| value "<< endl;
		}
		index++;
		//xmlnode = tem.Element();
		//if(xmlnode!=NULL) cout << xmlnode->Value() << endl;
	}

	return handle.Element();
}


TiXmlElement* config::creatElementFromPath(string path)
{
    string tempStr,file,nodeStr,attrName,attrStr;
	string node[MAX_NODE_DEPTH];
	unsigned int index,depth,start,end;
	int split;
	int unknown_node;
	TiXmlElement* xmlnode;
	index = 0;
	depth = 0;
	//cout << "Receive string:" << path << endl;
	while ((index < path.length()) && (depth < MAX_NODE_DEPTH))
	{
		start = index;
		while ((index < path.length()) && (path[index] != '\\')&& (path[index] != '.'))
			index++;
		end = index;

		if (depth == 0)
		{
			file = path.substr(start,end - start);
			//cout << "filename: " << file <<endl;
		}
		node[depth] = path.substr(start,end - start);
		//transform(node[depth] .begin(), node[depth] .end(), node[depth] .begin(), ::toupper);
		//printf("Session %d-%d index = %d depth=%d \r\n",start,end,index,depth);
		depth++;
		index++;
	}
	int id;
	TiXmlDocument *doc = getDocFromStr(file,id);
	if (doc == NULL)
	{
		cout << "Unable to find document.\r\n";
		return NULL;
	}
	xmls[id].changed=1;
	TiXmlHandle handle(doc);
	TiXmlHandle next(doc);
	index = 0;

	while (index<depth)
	{
        tempStr = node[index];
        attrName="";attrStr="";
        //cout << "str: " << tempStr << endl;
        if ((split = tempStr.find('@',0)) > 0)
		{
			//printf("Split %s at %d",tempStr.c_str(),split);
			nodeStr = tempStr.substr(0,split);
			attrName = tempStr.substr(split+1,tempStr.length());
			if ((split = attrName.find(':',0)) > 0)
			{
			    attrStr = attrName.substr(split+1,attrName.length());
			    attrName = attrName.substr(0,split);
			}
			//cout << "Node: " << nodeStr << " Attr: " << attrName << " AttrStr: " << attrStr << endl;
			transform(nodeStr.begin(), nodeStr.end(), nodeStr.begin(), ::toupper);
			transform(attrName.begin(), attrName.end(), attrName.begin(), ::toupper);

            xmlnode=NULL;
            next = handle.FirstChild(nodeStr.c_str());
            xmlnode = next.Element();
			//if((xmlnode == NULL)) cout << "null" << endl;
			unknown_node=1;
			while (xmlnode != NULL)
			{
				if (xmlnode->Attribute(attrName.c_str()) != NULL)
				{
				    if (attrStr.compare(xmlnode->Attribute(attrName.c_str())) == 0)
				    {
						//cout << xmlnode->Attribute(attrName.c_str())<<  endl;
						next = TiXmlHandle(xmlnode);
						unknown_node=0;
						break;
                    }
                    xmlnode = xmlnode->NextSiblingElement(nodeStr.c_str());
				}
				else
				{
				     xmlnode->SetAttribute(attrName.c_str(),attrStr.c_str());
				    next = TiXmlHandle(xmlnode);
                    unknown_node=0;
                    break;
				}
			}
			if(unknown_node)
			{
			    //cout << "add node @"<<next.Element()->Attribute(attrName.c_str())  <<endl;
			    TiXmlElement* newElem = new TiXmlElement(nodeStr.c_str());
			    newElem->SetAttribute(attrName.c_str(),attrStr.c_str());
			    handle.Element()->LinkEndChild(newElem);
			    next = newElem;
			}
			handle=next;
		}
		else
		{
		    transform(tempStr.begin(), tempStr.end(), tempStr.begin(), ::toupper);
			next = handle.FirstChild(tempStr.c_str());
			if(next.Element()==NULL && handle.Element()!=NULL)
			{
			    TiXmlElement* newElem = new TiXmlElement(tempStr.c_str());
			    handle.Element()->LinkEndChild(newElem);
			    next = handle.FirstChild(tempStr.c_str());
			}
			handle=next;
			//cout << "Session: |" << tempStr << "| value "<< endl;
		}
		index++;
	}
    return handle.Element();
}


TiXmlElement* config::getElementFromPathForSet(string path,int &id)
{
	string tempStr,file,nodeStr,attrName,attrStr;
    int split;
	string node[MAX_NODE_DEPTH];
	unsigned int index,depth,start,end;
	int unknown_node=0;
	TiXmlElement* xmlnode;
	index = 0;
	depth = 0;
	//cout << "Receive string:" << path << endl;
	while ((index < path.length()) && (depth < MAX_NODE_DEPTH))
	{
		start = index;
		while ((index < path.length()) && (path[index] != '\\')&& (path[index] != '.'))
			index++;
		end = index;

		if (depth == 0)
		{
			file = path.substr(start,end - start);
			//cout << "filename: " << file <<endl;
		}
		node[depth] = path.substr(start,end - start);
		//transform(node[depth] .begin(), node[depth] .end(), node[depth] .begin(), ::toupper);
		//printf("Session %d-%d index = %d depth=%d \r\n",start,end,index,depth);
		depth++;
		index++;
	}
	//sleep(5);
	TiXmlDocument *doc = getDocFromStr(file,id);
	if (doc == NULL)
	{
		cout << "Unable to find document.\r\n";
		return NULL;
	}
	TiXmlHandle handle(doc);
	TiXmlHandle tem(&xmls[id].template_doc);
	//TiXmlElement* xmlnode;
	index = 0;

	//cout << index << "AA" << depth ;
	while (index<depth)
	{
		tempStr = node[index];
		if ((split = tempStr.find('@',0)) > 0)
		{
			//printf("Split %s at %d",tempStr.c_str(),split);
			nodeStr = tempStr.substr(0,split);
			attrName = tempStr.substr(split+1,tempStr.length());
			if ((split = attrName.find(':',0)) > 0)
			{
			    attrStr = attrName.substr(split+1,attrName.length());
			    attrName = attrName.substr(0,split);
			}
			transform(nodeStr.begin(), nodeStr.end(), nodeStr.begin(), ::toupper);
			transform(attrName.begin(), attrName.end(), attrName.begin(), ::toupper);
			handle = handle.FirstChild(nodeStr.c_str());
			xmlnode = handle.Element();
			//if((xmlnode == NULL)) cout << "null" << endl;
			unknown_node=1;
			while (xmlnode != NULL)
			{
			    handle = NULL;
				if (xmlnode->Attribute(attrName.c_str()) != NULL)
				{
				    if (/*attrStr.compare("*") == 0 || */attrStr.compare(xmlnode->Attribute(attrName.c_str())) == 0)
				    {
						//cout << xmlnode->Attribute(attrName.c_str())<<  endl;
						handle = TiXmlHandle(xmlnode);
						unknown_node=0;
						break;
                    }
                    xmlnode = xmlnode->NextSiblingElement(nodeStr.c_str());
				}
				else xmlnode=NULL;
			}
			if(unknown_node)  break;
		}
		else
		{
		    transform(tempStr.begin(), tempStr.end(), tempStr.begin(), ::toupper);
			handle = handle.FirstChild(tempStr.c_str());
			tem=tem.FirstChild(tempStr.c_str());
			//cout << "Session: |" << tempStr << "| value "<< endl;
		}
		index++;
		//xmlnode = tem.Element();
		//if(xmlnode!=NULL) cout << xmlnode->Value() << endl;
	}

    if(unknown_node) return NULL;

    if( handle.Element()==NULL && tem.Element()!=NULL)
    {
        return creatElementFromPath(path);
    }
	return handle.Element();

}

TiXmlDocument* config::getDocFromStr(string str, int &id)
{
	TiXmlDocument* temp;
	transform(str.begin(), str.end(), str.begin(), ::toupper);
	//cout << str<< endl;
	if (str.compare("NETWORK") == 0)
	{
	    temp = &xmls[XML_NETWORK].doc;
	    id=XML_NETWORK;
	}
	else if (str.compare("DATAFLOW") == 0)
	{
	    temp = &xmls[XML_DATAFLOW].doc;
	    id=XML_DATAFLOW;
	}
	else if (str.compare("DEVICE") == 0)
	{
	    temp = &xmls[XML_DEVICE].doc;
	    id=XML_DEVICE;
	}
	else if (str.compare("RECORD") == 0)
	{
	    temp = &xmls[XML_RECORD].doc;
	    id=XML_RECORD;
	}
	else if (str.compare("GPS") == 0)
	{
	    temp = &xmls[XML_GPS].doc;
	    id=XML_GPS;
	}
	//else if (str.compare("GPSCARD") == 0)
	//	temp = &docGPSCard;
	else if (str.compare("NTRIP") == 0)
	{
	    temp = &xmls[XML_NTRIP].doc;
	    id=XML_NTRIP;
	}
	else if (str.compare("PORTS") == 0)
	{
	    temp = &xmls[XML_PORTS].doc;
	    id=XML_PORTS;
	}
	//else if (str.compare("REGISTRATION") == 0)
	//	temp = &docRegistration;
	//else if (str.compare("SCHEDULE") == 0)
	//	temp = &docSchedule;
	//else if (str.compare("SERVERS") == 0)
	//	temp = &docServers;
	else if (str.compare("SYSTEM") == 0)
	{
	    temp = &xmls[XML_SYSTEM].doc;
	    id=XML_SYSTEM;
	}

	else if (str.compare("USERS") == 0)
	{
	    temp = &xmls[XML_USERS].doc;
	    id=XML_USERS;
	}

	else if (str.compare("ANTENNALIST") == 0)
	{
	    temp = &xmls[XML_ANTENNALIST].doc;
	    id=XML_ANTENNALIST;
	}
	else if (str.compare("RADIO") == 0)
	{
	    temp = &xmls[XML_RADIO].doc;
	    id=XML_RADIO;
	}
	else if (str.compare("VPN") == 0)
	{
	    temp = &xmls[XML_VPN].doc;
	    id=XML_VPN;
	}
	else if (str.compare("NETDATA") == 0)
	{
	    temp = &xmls[XML_NETDATA].doc;
	    id=XML_NETDATA;
	}
	else
	{
	    temp = NULL;
	    id=XML_UNKNOWN;
	}

	return temp;
}



const char* config::getFullPath(char* fullpath, const char* filename)
{
	strcpy(fullpath, CONFIG_PATH);
	strcat(fullpath, filename);
	return fullpath;
}

void config::encrypt(const char *xmlFile)
{
    string xmlPath;
    xmlPath.assign(CONFIG_PATH);
    xmlPath.append(xmlFile);

    AES_Encrypt(xmlPath.c_str(),xmlPath.c_str(),HASH_SHA256);
}
