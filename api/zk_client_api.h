#ifndef _ZK_CLIENT_API_H_
#define _ZK_CLIENT_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper_log.h>

#define PATH_BUFFER_LEN 1024

void clients_watcher_g(zhandle_t * zh, int type, int state, const char* path, void* watcherCtx);
void election_watcher(zhandle_t * zh, int type, int state,const char* path, void* watcherCtx);
typedef void (*master_value_change_fun)(std::string);
typedef struct _watch_func_para_t {
        zhandle_t *zkhandle;
        char node[PATH_BUFFER_LEN];
} watch_func_para_t; 


typedef struct _name_service_t {
    char name[16];
    char zknode[PATH_BUFFER_LEN];
    char * hostip;
    int hostport;
} name_service_t;


class zkClient{
    public:
        zkClient();
        ~zkClient();

        void zkInit();
        int zkLoadConf(const char * filename);
        void zkGetLeader( std::string path, struct String_vector strings );

        std::string getMasterValue(){return master_value;}
	char* getErrorMsg(){ return error_msg; }

    public:
        master_value_change_fun func_master_value_onchange;

    private:

    private:
        //handle
        zhandle_t * zkhandle;

        //zookeeper
        std::string master_value;

        //config
        std::string host; 
        std::string root; 
        int timeout;

        std::map<std::string, std::string> config;
        char error_msg[1024];
};

#endif

