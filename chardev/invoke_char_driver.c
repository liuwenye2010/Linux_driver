#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

main(){
	int fd;
	char *hello_node0 = "/dev/chardevnode0";
	char *hello_node1 = "/dev/chardevnode1";
/*O_RDWR只读打开,O_NDELAY非阻塞方式*/	
	if((fd = open(hello_node0,O_RDWR|O_NDELAY))<0){
		printf("APP open %s failed!\n",hello_node0);
	}
	else{
		printf("APP open %s success!\n",hello_node0);
	}
	
	close(fd);
	
	if((fd = open(hello_node1,O_RDWR|O_NDELAY))<0){
		printf("APP open %s failed!\n",hello_node1);
	}
	else{
		printf("APP open %s success!\n",hello_node1);
	}
	
	close(fd);
	
}