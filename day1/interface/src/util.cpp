#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "24cXX.h"
#include "share.hpp"
#include "util.hpp"

using namespace std;


const unsigned char crc8table[256] = { //reversed, 8-bit, poly=0x07
  0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
  0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
  0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
  0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
  0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
  0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
  0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
  0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
  0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
  0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
  0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
  0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
  0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
  0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
  0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
  0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
  0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
  0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
  0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
  0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
  0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
  0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
  0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
  0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
  0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
  0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
  0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
  0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
  0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
  0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
  0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
  0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF };

unsigned char  cmd_crc8(unsigned char   *input, int count) {
  unsigned char  fcs = 0xFF;
  int i;
  for (i = 0; i < count; i++) {
    fcs = crc8table[fcs^input[i]];
  }
  return fcs;
}



unsigned char C2Hex(char *c)
{
    unsigned char d0=0,d1=0;
    if(c[0]>='0' && c[0]<='9') d0=c[0]-'0';
    else if(c[0]>='A' && c[0]<='F') d0=c[0]-'A'+10;
    else if(c[0]>='a' && c[0]<='f') d0=c[0]-'a'+10;

    if(c[1]>='0' && c[1]<='9') d1=c[1]-'0';
    else if(c[1]>='A' && c[1]<='F') d1=c[1]-'A'+10;
    else if(c[1]>='a' && c[1]<='f') d1=c[1]-'a'+10;

    return (d0<<4|d1);
}
/**字符串转HEX**/
unsigned int Sting2Hex(const char *str,unsigned int str_len,unsigned char *data,unsigned int max_size)
{
    unsigned int i;
    unsigned int size=0;

    for(i=0;i<(str_len-1);i+=2)
    {
        if(str[i]==' ')
        {
            i+=1;
            //continue;
        }
        if(size<max_size)
        {
           data[size++]=C2Hex((char *)&str[i]);
        }
        else
        {
            return 0;//return 0;
        }
    }

    return size;
}

void trim_string(char *str)
{
    char *start, *end;
    int len = strlen(str);

    //去掉最后的换行符
    if(str[len-1] == '\n')
    {
        len--;
        str[len] = 0;
    }

    //去掉两端的空格
    start = str;
    end = str + len -1;
    while(*start && isspace(*start))
        start++;
    while(*end && isspace(*end))
        *end-- = 0;
    strcpy(str, start);
}

bool Ctl_GetMachInfoValue(char* buffer,const char* name,char* value,const char* defaultValue)
{
  char searchStr[32];
  char* ptr1;
  char* ptr2;
  strcpy(searchStr,"[");
  strncat(searchStr,name,30);
  strcat(searchStr,"]=");
  ptr1 = strstr(buffer,searchStr);
  if (ptr1 != NULL)
  {
    ptr1 += strlen(searchStr);
    ptr2 = strchr(ptr1,'\r');
	if (ptr2 == NULL) ptr2 = strchr(ptr1,'\n');
    if (ptr2 == NULL) ptr2 = buffer+ strlen(buffer);
    strncpy(value,ptr1,ptr2-ptr1);
    value[ptr2-ptr1] = 0;
    trim_string(value);
    return true;
  }
  else
  {
    strcpy(value,defaultValue);
    return false;
  }
}

char *getlocalhostip(const char *net)
{
    /*
    #define ERRORIP "cannot find host ip"
    int sfd, intr;
    struct ifreq buf[16];
    struct ifconf ifc;
    sfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0)
        return (char *)ERRORIP;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
        return (char *)ERRORIP;
    intr = ifc.ifc_len / sizeof(struct ifreq);
    while (intr-- > 0 && ioctl(sfd, SIOCGIFADDR, (char *)&buf[intr]));
    close(sfd);
    return inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))-> sin_addr);
    */

    //#define READ_IP_CMD  "ifconfig eth0|awk -F\"[: ]+\" \'/inet addr/{print $4}\'"
	char READ_IP_CMD[256]={0};
	sprintf(READ_IP_CMD,"ifconfig %s|awk -F\"[: ]+\" \'/inet addr/{print $4}\'",net);
    FILE *fpin=NULL;
    static char str[128];
    memset(str,0,sizeof(str));
    if(NULL!=(fpin=popen(READ_IP_CMD,"r")))
    {
        if(NULL == fgets(str, sizeof(str), fpin))
        {
			str[strlen(str)-1]='\0';
        }
        pclose (fpin);
    }
    return str;
}

int OEM_Ping()
{
	FILE *fstream=NULL;
	char buff[32];
	int err=0;

	system("ping 192.167.100.190 -s 8000 -c 5 > /tmp/ping_tmp");

	memset(buff,0,sizeof(buff));

	fstream=popen("cat /tmp/ping_tmp|grep loss|awk '{print $7}'","r");
	if( NULL!=fgets(buff, sizeof(buff), fstream) ){
		if(strcmp(buff, "100%\n")==0){
			err=1;
		}
	}else {
		err=1;
	}
	system("rm /tmp/ping_tmp");
	pclose(fstream);

	return err;
}

 const char* stristr(const char* str, const char* subStr)
{
        int len = strlen(subStr);
        if(len == 0)
        {
            return NULL;
        }

        while(*str)
        {
            if(strncasecmp(str, subStr, len) == 0)
            {
                return str;
            }
            ++str;
        }
        return NULL;
}

int get_keyword(char *str, char *key, char *value, int n)
{
    int len=0;
    char *p;
    memset(value,0,n);
    if((p=strstr(str,key))>0)
    {
        p+=strlen(key);
        //printf("p|%s|\n",p);
        while(*p==' ') p++;
        while(*p)
        {
            if(*p=='\n' || *p=='\r')
            {
                return len;
            }
            if(len<n) value[len]=*p;
            len++;
            p++;
        }
    }
    return 0;
}

bool get_version_string(string version,string *res)
{
    static char buf[128];
    sprintf(buf,"%.02f",atof(version.c_str())/100);
    res->assign(buf);
    return true;
}

bool get_sys_versions(string* os, string* uboot, string* uimage)
{
    //cat /proc/version
    #define VER_CMD  "cat /proc/version"
    #define OS_KEYWORD                 "Linux version"
    #define UBOOT_KEYWORD       "u-boot version"
    #define UIMAGE_KEYWORD      "uImage version"

    FILE *fpin=NULL;
    char str[512];
    char buf[128];
    memset(str,0,sizeof(str));
    if(NULL!=(fpin=popen(VER_CMD,"r")))
    {
            while(NULL != fgets(str+strlen(str), sizeof(str)-strlen(str), fpin));

            pclose (fpin);
            //printf("str|%s|\n",str);
            get_keyword(str,(char *)OS_KEYWORD,buf,sizeof(buf));
            os->assign(buf);
            get_keyword(str,(char *)UBOOT_KEYWORD,buf,sizeof(buf));
            uboot->assign(buf);
            get_keyword(str,(char *)UIMAGE_KEYWORD,buf,sizeof(buf));
            uimage->assign(buf);
            return true;
    }
    return false;
}

bool get_sys_imei(string* imei)
{
    FILE *fpin=NULL;
    char buf[16];
    int ret=0;

    fpin = fopen("/geo/app/IMEI", "r");
    if( NULL==fpin ){
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    ret=fread(buf, 15, 1, fpin);
    printf("ret:%d buf:%s\n", ret, buf);
    if(ret>0){
        imei->assign(buf);
    }
    fclose(fpin);
    return true;
}

bool ucshell(string cmd, string* ret)
{
    string tmp;
    FILE *fpin=NULL;
    char str[512];
    ret->clear();
    if(cmd.length()==0) return false;
    if(NULL!=(fpin=popen(cmd.c_str(),"r")))
    {
            memset(str,0,sizeof(str));
            while(NULL != fgets(str, sizeof(str)-1, fpin))
            {
                tmp.assign(str);
                ret->append(string_replace(tmp,"\n","\r\n"));
            }
            pclose (fpin);
            return true;
    }

    return false;
}

bool get_mac(string* mac)
{
    /* /etc/bt_addr */
    char buf[128];
    int fd ;
    char *p;
    memset(buf,0,sizeof(buf));
    fd = open("/etc/bt_addr",O_RDONLY) ;
    if ( fd == -1 )
    {
        return false;
    }
    if(read(fd,buf,sizeof(buf))>0)
    {
        p=buf;
        while(*p)
        {
            if(*p=='\n' || *p=='\r') *p=0;
            p++;
        }
         mac->assign(buf);
    }

    return true;
}

bool get_dev_serial(string *serial)
{
    unsigned int i;
    struct eeprom e;
    char buf[64];
#ifdef  NSC200
    if(eeprom_open((char *)"/dev/i2c-1", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#else
    if(eeprom_open((char *)"/dev/i2c-2", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#endif
    memset(buf,0,sizeof(buf));
	for(i=0;i<sizeof(buf);i++)
	{
        buf[i]=eeprom_read_byte(&e,i);
        if(!(buf[i]>='0' && buf[i]<='Z')) break;
	}
	//printf("i2c: %s\n",buf);
	buf[14]=0;
	serial->assign(buf);
	return true;
}

bool get_disk_size(string path,string *size,string *free)
{
    char buf[128];
    struct statfs diskInfo;
    statfs(path.c_str(),&diskInfo);
    unsigned long long blocksize = diskInfo.f_bsize;// 每个block里面包含的字节数
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;//总的字节数
    //printf("TOTAL_SIZE == %llu B\n",totalsize); // 1024*1024 =1MB  换算成MB单位
    unsigned long long freeDisk = diskInfo.f_bfree*blocksize; //再计算下剩余的空间大小
    //printf("DISK_FREE == %llu B\n",freeDisk);

    sprintf(buf,"%llu",totalsize);
	size->assign(buf);
	sprintf(buf,"%llu",freeDisk);
	free->assign(buf);
	return true;
}

bool get_time(string *t)
{
    struct tm  *ptm;
    long   ts;
    ts = time(NULL);
    ptm = localtime(&ts);
	char tempStr[128];
	sprintf(tempStr,"%04d-%02d-%02d %02d:%02d:%02d",
              ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,
              ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
    t->assign(tempStr);
    return true;
}

string&   string_replace(string&   str,const   string&   old_value,const   string&   new_value)
{
    for(string::size_type   pos(0);   pos!=string::npos;   pos+=new_value.length())   {
        if(   (pos=str.find(old_value,pos))!=string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }
    return   str;
}


bool  check_usbconnect()
{
    unsigned char buf[2]={0,0};
    int fd = open("/dev/otg",O_RDWR);
    read(fd, buf, 1);
    close(fd);
    if(buf[0]==1) return true;
    return false;
}


int start_process(string cmd)
{
    //process
    int process_id=0;
    pid_t pid;
    pid=fork();
    if (pid == -1)
    {
        //perror("fork() error");
        syslog(LOG_LOCAL7|LOG_ERR,"start_process()  fork error!");
        return 0;
    }
    if (pid == 0)
    {
        system(cmd.c_str());
        exit(0);
    }
    if (pid > 0)
    {
        process_id=pid ;
    }
    return process_id;
}

int  stop_process(int id)
{
    int n=0,status;
    if(id)
    {
        int ret=kill( id,SIGTERM );
        if ( ret )
        {
            id=0;
            return false;
        }
        else
        {
            while(waitpid( id, &status, WNOHANG )==0&&n++<100)
            {
                usleep(10000);
                //syslog(LOG_LOCAL7|LOG_INFO,"######## %d ########\n",n);
            }

            if(n>=100)
            {
                kill( id,SIGKILL );
                syslog(LOG_LOCAL7|LOG_INFO,"######## %d ########\n",n);
            }

            id=0;
        }
    }
    return id;
}

static int clean_process_id=0;
bool clean_process()
{
    stop_process(clean_process_id) ;
    clean_process_id=start_process(CLEAN_BIN_PATH);
    syslog(LOG_LOCAL7|LOG_WARNING,"auto clean !");
    return (clean_process_id>0);
}


int str_count(const char* str,const char* s)
{
    char* s1;
    char* s2;
    int count = 0;
    while(*str!='\0')
    {
        s1 = (char*)str;
        s2 = (char*)s;
        while(*s2 == *s1&&(*s2!='\0')&&(*s1!='0'))
        {
            s2++;
            s1++;
        }
        if(*s2 == '\0')
            count++;
        str++;
    }
    return count;
}


int isdir(const char *path)
{
    struct stat st;
    stat(path, &st);
    return (S_ISDIR(st.st_mode));
}

int deletedir(const char *path)
{
    int n=0;
    DIR   *dir_p;
    struct   dirent   *entry;
    char tmp[256];

     if ((dir_p=opendir(path))==NULL)
    {
        return (-1);
    }

    while ((entry=readdir(dir_p))!=NULL)
    {
         if(strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
         {
            sprintf(tmp,"%s/%s",path,entry->d_name);
            if (isdir(tmp))
            {
                n+=deletedir(tmp);
            }
            else
            {
                unlink(tmp);
                n++;
            }
         }
    }
    closedir(dir_p);
    rmdir(path);
    return (n);
}




bool eeprom_read(unsigned short addr,unsigned char *data,int len)
{
    int i;
    struct eeprom e;
#ifdef  NSC200
    if(eeprom_open((char *)"/dev/i2c-1", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#else
    if(eeprom_open((char *)"/dev/i2c-2", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#endif
	for(i=0;i<len;i++)
	{
        data[i]=eeprom_read_byte(&e,addr+i);
	}
	eeprom_close(&e);
	return true;
}


bool eeprom_write(unsigned short addr, unsigned char *data,int len)
{
    int i;
    struct eeprom e;
#ifdef  NSC200
    if(eeprom_open((char *)"/dev/i2c-1", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#else
    if(eeprom_open((char *)"/dev/i2c-2", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#endif
	for(i=0;i<len;i++)
	{
        if(eeprom_write_byte(&e,addr+i,data[i])<0)
        {
            syslog(LOG_LOCAL7|LOG_ERR,"write eeprom error at %d+%d",addr,i);
            eeprom_close(&e);
            return false;
        }
	}
	eeprom_close(&e);
#if 0
#ifdef  NSC200
    if(eeprom_open((char *)"/dev/i2c-1", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#else
    if(eeprom_open((char *)"/dev/i2c-2", 0x51, EEPROM_TYPE_16BIT_ADDR, &e))
	{
	   syslog(LOG_LOCAL7|LOG_ERR,"open eeprom error");
	   return false;
	}
#endif
	for(i=0;i<len;i++)
	{
        if(eeprom_read_byte(&e,addr+i)!=data[i])
        {
            syslog(LOG_LOCAL7|LOG_ERR,"check eeprom error at %d+%d",addr,i);
            eeprom_close(&e);
            return false;
        }
	}
	eeprom_close(&e);
#endif
	return true;
}


bool update_authcode(char *code)
{
    unsigned short length = strlen(code);
    return eeprom_write(REGI_ADDR,(unsigned char *)code,length);
}

unsigned char authcode[64];
extern "C" { int ValidateSN(unsigned char *pData,char *pDeviceID,unsigned int *pExpireDateTime,unsigned char *option);}
bool check_authcode(char *DeviceID,unsigned int *ExpireDateTime,unsigned char *Option)
{
    int i;
    memset(authcode,0,sizeof(authcode));
    if(!eeprom_read(REGI_ADDR,(unsigned char *)authcode,32))
    {
        syslog(LOG_LOCAL7|LOG_ERR,"eeprom_read error!");
        return false;
    }
    printf("1 authcode = %s\n", authcode);
    printf("authcode[0]: %d  authcode[1]: %d\n", authcode[0], authcode[1]);
    //syslog(LOG_LOCAL7|LOG_INFO,"auth code %s",authcode);
    for(i=0; i<32; i++){
        if(('0'<=authcode[i]&&authcode[i]<='9')||
           ('a'<=authcode[i]&&authcode[i]<='w')||
           ('A'<=authcode[i]&&authcode[i]<='W'))
           continue;
        else
            authcode[i]=' ';
    }

    printf("2 authcode = %s\n", authcode);
    *ExpireDateTime=0;
    *Option=0;
    if(ValidateSN(authcode,DeviceID,ExpireDateTime,Option))
    {
        return false;
    }
    return true;
}

bool test_authcode(char *authcode,char *DeviceID,unsigned int *ExpireDateTime,unsigned char *Option)
{
    *ExpireDateTime=0;
    *Option=0;
    if(ValidateSN((unsigned char *)authcode,DeviceID,ExpireDateTime,Option))
    {
        return false;
    }
    return true;
}



int  file_read(const char *path,unsigned char *data,int len)
{
    int file;
    int n=0;
    file = open(path, O_RDWR,0666);
    if (file < 0)
    {
        return -1;
    }
    n=read(file,data,len);
    close(file);
    return n;
}

int  file_write(const char *path,unsigned char *data,int len)
{
    int file;

    if((access(path,R_OK))==0)
    {
        unlink(path);
    }

    file = open(path, O_RDWR|O_CREAT,0666);
    if (file < 0)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"file_write() open file error: %s",path);
        return -1;
    }
    if(write(file, data,len)!=len)
    {
        syslog(LOG_LOCAL7|LOG_ERR,"file_write() write file error: %s",path);
        close(file);
        return -1;
    }
    fsync(file);
    close(file);

    return len;
}

bool device_poweroff()
{
    unsigned char tmp[2];
    int fd;
    char data[1]={0x0};
	int systemRet;
    sync();
    fd = open("/dev/msp430", O_RDWR);
    printf("Kill all processes\n");
    printf("Sending all processes the TERM signal...\n");
    systemRet=system("killall5 -15");
	if(systemRet == -1)
	{
		perror("killall5 -15 failed");
	}
    systemRet=system("umount /dev/mmcblk0p4");
	if(systemRet == -1)
	{
		perror("umount /dev/mmcblk0p4 failed");
	}
    systemRet=system("umount /dev/mmcblk0p4");
	if(systemRet == -1)
	{
		perror("umount /dev/mmcblk0p4 failed");
	}
    systemRet=system("umount /dev/mmcblk0p4");
	if(systemRet == -1)
	{
		perror("umount /dev/mmcblk0p4 failed");
	}
    usleep(200*1000);
    sync();
    printf("Sending all processes the KILL signal...\n");
    systemRet=system("killall5 -9");
	if(systemRet == -1)
	{
		perror("killall5 -9 failed");
	}
    sync();

    systemRet=system("/etc/rc6.d/S25save-rtc.sh");
	if(systemRet == -1)
	{
		perror("/etc/rc6.d/S25save-rtc.sh failed");
	}
    systemRet=system("/etc/rc6.d/S40umountfs");
	if(systemRet == -1)
	{
		perror("/etc/rc6.d/S40umountfs failed");
	}
    sync();
    usleep(400000);
	int ret=0;
    ret=write(fd, data, 1);
	if(ret==0)
	{
		perror("write error");
	}

    return true;
}

template<typename Out>
void split(const std::string &s, char delim, Out result)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

int _split(char *src, const char seperator, char **pCols, const int nMaxCols)
{
	char *p;
	char **pCurrent;
	int count = 0;

	if (nMaxCols <= 0)
	{
		return 0;
	}
	p = src;
	pCurrent = pCols;
	while (1)
	{
		*pCurrent = p;
		pCurrent++;
		count++;
		if (count >= nMaxCols)
		{
			break;
		}
		p = strchr(p, seperator);
		if (p == NULL)
		{
			break;
		}
		*p = '\0';
		p++;
	}
	return count;
}

int isempty(char *path)
{
	DIR *dirp;
	int num=0;

	dirp = opendir(path);
	while (dirp)
	{
		if ( readdir(dirp) != NULL)// . ..   num=2
			++num;
		else
			break;
	}

	closedir(dirp);
	if(num==2)return 1;
	else return 0;
}

int is_validIPv4Address(const char *ip_str)
{
	struct in_addr addr;
	int ret;
	volatile int local_errno;

	if(ip_str[0]==48)//'0'
	{
		return -1;
	}

	errno = 0;
	ret = inet_pton(AF_INET, ip_str, &addr);
	local_errno = errno;
	if (ret > 0)
		fprintf(stderr, "\"%s\" is a valid IPv4 address\n", ip_str);
	else if (ret < 0)
		fprintf(stderr, "EAFNOSUPPORT: %s\n", strerror(local_errno));
	else
		fprintf(stderr, "\"%s\" is not a valid IPv4 address\n", ip_str);

	/*u_int32_t p1, p2, p3, p4;
	if (sscanf(ip_str, "%u.%u.%u.%u", &p1, &p2, &p3, &p4) != 4)
		return 0;
	if (p1 <= 255 && p2 <= 255 && p3 <= 255 && p4 <= 255) {
		char tmp[64];
		sprintf(tmp, "%u.%u.%u.%u", p1, p2, p3, p4);
		return !strcmp(tmp, ip_str);
	}
	return 0;*/

	return ret;
}
