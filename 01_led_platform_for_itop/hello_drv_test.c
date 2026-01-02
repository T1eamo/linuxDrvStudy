#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv)
{
	int fd;
	char buf[1024];
	int len;
	if(argc < 2){
		printf("the argc is %d\n",argc);
		return -1;
	}
	fd = open("/dev/hello",O_RDWR);
	if(fd == -1){
		printf("can not find the /dev/hello\n");
		return -1;
	}
	if ((0 == strcmp(argv[1],"-w")) && (argc == 3)){
		len = strlen(argv[2])+1;
		write(fd,argv[2],len);
	}else{
		len = read(fd,buf,1024);
		buf[1023] = '\0';
		printf("APP read :%s\n",buf);
	}
	close(fd);
	return 0;
	

}




