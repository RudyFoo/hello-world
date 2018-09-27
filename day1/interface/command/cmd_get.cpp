/*
 * cmd_get.cpp
 *
 *  Created on: 2009-7-28
 *      Author: Administrator
 */
#include "command.hpp"

int command::do_GET(int argc, string (&argv)[MAX_CMD_ARGS])
{
	string value;
	int result;
	//cout<<argv[1]<<endl;
	result = xmlconfig->getValue(argv[1],&value);
	if (result == RET_OK)
		ShowMsgLine(argv[0] + ","+argv[1]+ ",OK," + value);
	return result;
}

