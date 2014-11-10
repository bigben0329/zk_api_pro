#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "zk_server_api.h"

#define BUFFER_LEN 1024

static struct in_addr get_local_ip() {                                                                                                  
    int fd; 
    struct ifreq ifreq;
    struct sockaddr_in* sin;

    fd = socket(PF_INET, SOCK_DGRAM, 0); 
    memset(&ifreq, 0x00, sizeof(ifreq));
    strncpy(ifreq.ifr_name, "eth1", sizeof(ifreq.ifr_name));
    ioctl(fd, SIOCGIFADDR, &ifreq);
    close(fd);
    sin = (struct sockaddr_in* )&ifreq.ifr_addr;

    return sin->sin_addr; /* .s_addr; */
}


void run() {
    printf(".");
}

int main(int argc, const char * argv[]) {
    char data[BUFFER_LEN] = {0};
    int pid = getpid();
    sprintf(data, "data:%s:%d:%d", inet_ntoa(get_local_ip()), 19999); 

    zkServer zks;
    zks.zkLoadConf("/data/coc/conf/zk.conf");
    zks.zkInit();

    char path_buffer[BUFFER_LEN] = {0};
    zks.zkCreate( data, path_buffer, BUFFER_LEN);
    if(zks.isLeader())
    {
        printf("I'm master! path_buffer %s \n", path_buffer);
    }
    else
    {
        printf("I'm slave! path_buffer %s \n", path_buffer);
    }
    zks.watchChildren();

    int total = 2000;
    printf("Server is running...");
    while (total-- > 0) {
        run();
        sleep(1);
    }

    getchar();
    return 0;
}
