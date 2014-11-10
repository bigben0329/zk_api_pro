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
        int isLeader();
        int zkLoadConf(const char*);

        void setName(const std::string name);
        void setHost(const std::string host);
	char* getErrorMsg(){ return error_msg; }

    private:
        //load config attr
        std::string server_name;
        std::string host;
        std::string prefix;
        std::string root;
        int timeout;  

        //zookeeper value
        std::string node_name;
        zhandle_t * zkhandle;

        std::map<std::string, std::string> config;
	char error_msg[1024];
};

#endif

