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
    int port  = 799;
    char data[BUFFER_LEN] = {0};
    char path_buffer[BUFFER_LEN] = {0};

    zkServer zkserver("test");
    zkserver.zkLoadConf("conf/zk.conf");
    zkserver.set_host("127.0.0.1:2181");
    zkserver.zkInit();

    int pid = getpid();
    sprintf(data, "%s:%d:%d", inet_ntoa(get_local_ip()), port, pid); 

    zkserver.zkCreate( data, path_buffer, BUFFER_LEN);
    if(zkserver.is_leader())
    {
        printf("I'm master!\n");
    }
    else
    {
        printf("I'm slave!\n");
    }
    zkserver.watchChildren();

    int total = 200;
    printf("Server is running...");
    while (total-- > 0) {
        run();
        sleep(1);
    }

    getchar();


    return 0;
}
