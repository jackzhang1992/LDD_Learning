#include<sys/types.h>
#include<unistd.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
int main()
{
    int fd;
    char msg[100];
    fd= open("/dev/globalmem",O_RDWR,S_IRUSR|S_IWUSR);
    if(fd!=-1)
    {
        while(1)
        {
            printf("Please input the globar:\n");
            scanf("%s",msg);
            write(fd,msg,strlen(msg));
            if(strcmp(msg,"q")==0)
            {
                close(fd);
                break;
            }
        }
    }
    else
    {
        printf("device open failure\n");
    }
    return 0;
}
