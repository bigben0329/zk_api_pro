#ifndef  _ZK_SERVER_API_H_
#define _ZK_SERVER_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper_log.h>

class zkServer{
    public:
        zkServer();
        zkServer(std::string name);
        ~zkServer();

        int zkInit();
        int zkCreate( const char *value, char *path_buffer, int path_buffer_len);
        int watchChildren();
        int is_leader();
        void zkLoadConf(const char*);

        void set_name(const std::string name);
        void set_host(const std::string host);
    private:
        std::string serName;

        std::string zkHost;
        std::string zkNodePath;
        std::string zkRootPath;

        std::string zkNode;
        zhandle_t * zkhandle;
        int Timeout;  
        std::map<std::string, std::string> config;
};

#endif

