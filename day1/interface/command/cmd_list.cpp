/*
 * cmd_list.cpp
 *
 *  Created on: 2009-8-3
 *      Author: zekunyao
 */
#include "command.hpp"

int command::do_LIST(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;


	string value;
	result = xmlconfig->getList(argv[1],&value);
	if (result == RET_OK)
        ShowMsgLine(argv[0] + ","+ argv[1]+ ",OK," + value);

	return result;
}

