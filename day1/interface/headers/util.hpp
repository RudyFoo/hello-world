#ifndef _UTIL_H_
#define _UTIL_H_

#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <vector>
#include <iterator>
#include "sm4Func.h"

using namespace std;
unsigned char  cmd_crc8(unsigned char   *input, int count);
unsigned int Sting2Hex(const char *str,unsigned int str_len,unsigned char *data,unsigned int max_size);
bool Ctl_GetMachInfoValue(char* buffer,const char* name,char* value,const char* defaultValue);
const char* stristr(const char* str, const char* subStr);
char *getlocalhostip(const char *net);
bool get_sys_versions(string* os, string* uboot, string* uimage);
bool get_version_string(string version,string *res);
bool get_mac(string* mac);
bool ucshell(string cmd, string* ret);
bool get_dev_serial(string *serial);
bool get_disk_size(string path,string *size,string *free);
bool get_time(string *t);
bool get_sys_imei(string* imei);
string&   string_replace(string&   str,const   string&   old_value,const   string&   new_value);
std::vector<std::string> split(const std::string &s, char delim);
int _split(char *src, const char seperator, char **pCols, const int nMaxCols);
bool  check_usbconnect();
int start_process(string cmd);
int  stop_process(int id);
bool clean_process();
int str_count(const char* str,const char* s);
int isdir(const char *path);
int deletedir(const char *path);
bool device_poweroff();

int  file_read(const char *path,unsigned char *data,int len);
int  file_write(const char *path,unsigned char *data,int len);
int  isempty(char *path);
int is_validIPv4Address(const char *str);
int OEM_Ping();

bool eeprom_read(unsigned short addr,unsigned char *data,int len);
bool eeprom_write(unsigned short addr, unsigned char *data,int len);
bool update_authcode(char *code);
bool check_authcode(char *DeviceID,unsigned int *ExpireDateTime,unsigned char *Option);
bool test_authcode(char *authcode,char *DeviceID,unsigned int *ExpireDateTime,unsigned char *Option);
#endif
