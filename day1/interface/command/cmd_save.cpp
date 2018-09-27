/*
 * cmd_save.cpp
 *
 *  Created on: 2009-8-19
 *      Author: zekunyao
 */
#include "command.hpp"
int command::do_SAVE(int argc, string (&argv)[MAX_CMD_ARGS])
{
	int result;
	result = xmlconfig->saveConfig();
	if (result == RET_OK)
		ShowMsgLine("SAVE,OK");
	else
	{
	    ShowMsgLine("SAVE,ERROR");
	    syslog(LOG_LOCAL7|LOG_ERR,"save config error!");
	}

	return result;
}
