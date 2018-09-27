/*
 * cmd_save.cpp
 *
 *  Created on: 2009-8-19
 *      Author: zekunyao
 */
#include "command.hpp"
int command::do_UPDATE(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;
	result = xmlconfig->saveRealtime();
	//sync();
	if (result == RET_OK)
		ShowMsgLine("UPDATE,OK");
	return result;
}


int command::do_ADD(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;

	result = xmlconfig->addNode(argv[1]);
	if (result == RET_OK)
        ShowMsgLine(argv[0] + ","+ argv[1]+ ",OK" );

	return result;
}

/*
int command::do_DEL(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;

	result = xmlconfig->deleteNode(argv[1]);
	if (result == RET_OK)
        ShowMsg(argv[0] + ","+ argv[1]+ ",OK" );

	return result;
}*/
int command::do_DEL(int argc, string (&argv)[MAX_CMD_ARGS])
{
    int result;
    unsigned int index;
    unsigned int start,end;
    int split;
    string line=argv[1];
    string word;
    index = 0;
    nodes=0;
    transform(line.begin(), line.end(), line.begin(), ::toupper);
    //cout<<"line : " + line <<endl;
    while ((nodes < MAX_CMD_NODE) && (index < line.length()))
    {
        start = index;
        while ((index < line.length()) && ((line[index] != '.')))
        {
            index++;
        }
        end = index;
        word = line.substr(start,end - start);
        if ((split = word.find('@',0)) > 0)
		{
			word = word.substr(0,split);
		}
        node[nodes]=GetEnumFromKeyword((char *)word.c_str());
        //cout<<"node : " + word <<endl;
        //printf("enpath: %d\n",node[nodes]);
        nodes++;
        index++;
    }
    //printf("nodes: %d\n",nodes);
    if(nodes<2)
    {
        return RET_INVALID_NODE;
    }
    param.clear();
    param=(argv[1]);
    result=Del_Dowork();
    if(result==-1)
    {
        result = xmlconfig->deleteNode(argv[1]);
    }
    if (result == RET_OK)
    {
            ShowMsgLine(argv[0] + ","+ argv[1]+ ",OK" );
    }


    return result;

}



int command::Del_Dowork()
{
    if(nodes > 0)
    {
        enPathField enpath =(enPathField)node[0];
        //printf("node0 %d \n", enpath) ;
        switch(enpath)
        {
        case CMD_NTRIP:
            return Del_Ntrip();
        case CMD_NETWORK:
            return Del_NAT();
        case CMD_RECORD:
            return Del_Record();
        default:
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

int command::Del_NAT()
{
    enPathField enpath =(enPathField)node[1];
    //printf("node0 %d \n", enpath) ;

    switch(enpath)
    {
    case CMD_DNAT:
        return Del_Dnat();
    case CMD_SNAT:
        return Del_Snat();
    default:
        return -1;
    }
    return -1;
}

int set_dnat_stop(int id)
{
    return 0;
}
int set_dnat_start(int id)
{
    return 0;
}

int command::Del_Dnat()
{
    int split;
    int i,n,id=-1;
    char tmp[128], tmpStr[128];
    string str;
    string value=param; //NETWORK.DNAT.DNAT_LIST@ID:3
    if ((split = param.find(':',0)) > 0)  //查找':'的位置
    {
        id=atoi(param.substr(split+1,param.length()).c_str());
    }
    if(id>=0)
    {
        set_dnat_stop(id);
    }

    int result=xmlconfig->deleteNode(value);
    xmlconfig->deleteNode("DATAFLOW."+value);
    if(result) return result;

    if ((split = param.find('@',0)) > 0)
    {
        value = param.substr(0,split);
    }
    if ((split = param.find(':',0)) > 0)
    {
        id=atoi(param.substr(split+1,param.length()).c_str());
    }

    xmlconfig->setNodeAttById("NETWORK.DNAT.DNAT_LIST",n);  //这里赋值 n，
    //printf("====================\n");
    //cout<<n<<endl;
    //printf("====================\n");
    sprintf(tmpStr,"%d",n);
    xmlconfig->setValue("NETWORK.DNAT.TOTEL_LIST",tmpStr);  //这里赋值 n，
    //xmlconfig->setNodeAttById("DATAFLOW.NTRIP.CONNECTION",split);
    //sprintf(tmp,"%d",n);
    //value.assign(tmp);

    //xmlconfig->setValue("NETWORK.DNAT.DNAT_LIST",value);
    xmlconfig->saveConfig();

     if(id>=0)
     {
        if(n>id)
        {
            for(i=(id+1);i<(n+1);i++)
            {
                sprintf(tmp,"%d",(i-1));
                str.assign("NETWORK.DNAT.DNAT_LIST@ID:");
                str.append(tmp);

                xmlconfig->getValue(str+".ENABLE ",&value);
                transform(value.begin(), value.end(), value.begin(), ::toupper);
                cout<<value<<endl;

                if(value.compare("YES") == 0)
                {
                    set_dnat_stop(i);
                    set_dnat_start(i-1);
                }

            }
        }
     }

    return RET_OK;
}

int command::Del_Snat()
{
    return 0;
}

int command::Del_Record()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_SESSION:
        return Del_Record_Session();
    default:
        return -1;
    }
    return -1;
}

int command::Del_Record_Session()
{
    int split;
    string name;
    string value=param;
    if ((split = param.find(':',0)) > 0)
    {
        name=param.substr(split+1,param.length());
    }

    set_record_stop(name);

    char delCmd[MAX_PATH];

    //sprintf(delCmd,"rm -rf %s%s.txt",SESSION_FILE_LOG_DIR,name.c_str());
    //system(delCmd);
    sprintf(delCmd,"rm -rf %s%s.push",SESSION_FILE_LOG_DIR,name.c_str());
    system(delCmd);

    int result=xmlconfig->deleteNode(value);
    xmlconfig->deleteNode("DATAFLOW."+value);
    if(result) return result;

    xmlconfig->saveConfig();

    return RET_OK;
}

int command::Del_Ntrip()
{
    enPathField enpath =(enPathField)node[1];
    //printf("Record %d \n", enpath) ;
    switch(enpath)
    {
    case CMD_CONNECTION:
        return Del_Ntrip_Connection();
    default:
        return -1;
    }
    return -1;
}

int command::Del_Ntrip_Connection()
{
    int split;
    int i,n,id=-1;
    char tmp[128];
    string str;
    string value=param;
    if ((split = param.find(':',0)) > 0)
    {
        id=atoi(param.substr(split+1,param.length()).c_str());
    }
    if(id>=0)
     {
        set_ntrip_disconnect(id);
     }
    int result=xmlconfig->deleteNode(value);
    xmlconfig->deleteNode("DATAFLOW."+value);
    if(result) return result;

    if ((split = param.find('@',0)) > 0)
    {
        value = param.substr(0,split);
    }
    if ((split = param.find(':',0)) > 0)
    {
        id=atoi(param.substr(split+1,param.length()).c_str());
    }

    xmlconfig->setNodeAttById("NTRIP.CONNECTION",n);
    xmlconfig->setNodeAttById("DATAFLOW.NTRIP.CONNECTION",split);
    sprintf(tmp,"%d",n);
    value.assign(tmp);
    xmlconfig->setValue("NTRIP.CUR_CONNECTION",value);
    xmlconfig->saveConfig();

     //todo disconnect and connect
     if(id>=0)
     {
        if(n>id)
        {
            for(i=(id+1);i<(n+1);i++)
            {
                sprintf(tmp,"%d",(i-1));
                str.assign("NTRIP.CONNECTION@ID:");
                str.append(tmp);
                xmlconfig->getValue("DATAFLOW."+str+".STATUS",&value);
                transform(value.begin(), value.end(), value.begin(), ::toupper);
                cout<<value<<endl;
                if(value.compare("TRANSMITTING") == 0)
                {
                    set_ntrip_disconnect(i);
                    set_ntrip_connect(i-1);
                }
            }
        }
     }

    return RET_OK;
}
