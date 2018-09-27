//测试，在一个不存在的路径下面创建文件
//结论：不能成功
//
#include<stdio.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/types.h>  /*提供类型pid_t,size_t的定义*/

#include<sys/stat.h>

#include<fcntl.h>
int main()
{
    int fd=open("./shasha/a.txt",O_CREAT|O_RDWR);
    write(fd,"an dog",6);
    close(fd);
}
