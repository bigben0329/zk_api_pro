#include "zk_client_api.h"
#include "util.h"

#define DEBUG_PRINT(format, ...) printf( "#DEBUG[%s][%s][%u] "format"\n", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define ERROR_PRINT(format, ...) \
{ \
printf( "#ERROR %s %s "format"\n", __FILE__, __FUNCTION__, __VA_ARGS__); \
snprintf(error_msg, sizeof(error_msg), "#ERROR[%s][%s][%u] "format"\n", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
}

using namespace std;

void onmaster_valueChanged(std::string str)
{
    DEBUG_PRINT("!!!!!!!!!!!!!!! onmaster_valueChanged str %s", str.c_str());
}

zkClient::zkClient()
{
    timeout = 50000;
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    func_master_value_onchange = onmaster_valueChanged;

}

zkClient::~zkClient()
{
    if(NULL != zkhandle)
    {
        zookeeper_close(zkhandle);
    }
}

int zkClient::zkLoadConf(const char * filename)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE * fp = fopen(filename, "r");
    if (fp == NULL)
    {
        ERROR_PRINT("open config file %s error!\n", filename);
        return EXIT_FAILURE;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (line[0]=='#') // 跳过注释
            continue;
        DEBUG_PRINT("[%zu]line:%s", read, line);
        char *token, *key;
        int i = 0;
        while ((token = strsep(&line, "=")) != NULL) {
            if ( i==0 ) {
                key = token;
                i++;
            }
            else if ( i==1 ) {
                config[trim(key)] = trim(token);
            }
        }
    }
    if (line) {
        free(line);
    }

    fclose(fp);

    //load host
    host = config["host"];
    root = config["zkroot"];
    timeout = atoll(config["timeout"].c_str());

    if(host.empty()) host = "0.0.0.0:1234";
    DEBUG_PRINT("host %s", host.c_str());
    if(root.empty()) root = "root";
    DEBUG_PRINT("root %s", root.c_str());
    if(0 == timeout) timeout = 3000;
    DEBUG_PRINT("timeout %u", timeout);

    return 0;
}


void zkClient::zkInit()
{
    std::string &ss = config["service"];
    std::vector<std::string> slist = split(ss, ',');
    int service_len = slist.size();
    name_service_t * services = (name_service_t *)malloc( service_len * sizeof(name_service_t) );

    for ( int i=0; i<service_len; i++ ) {
        strcpy(services[i].name, slist[i].c_str());
        std::string tmp = root;
        tmp += slist[i];
        strcpy(services[i].zknode, tmp.c_str());
        services[i].hostip = NULL;
        services[i].hostport = 0;
    }

    DEBUG_PRINT("host:%s", host.c_str()); 
    zkhandle = zookeeper_init(host.c_str(), clients_watcher_g, timeout, 0, NULL, 0);
    if ( zkhandle==NULL )
    {
        DEBUG_PRINT("error connect to zookeeper service.","");
        return;
    }

    for ( int i=0; i<service_len; i++ ) {
        DEBUG_PRINT("[%s]", services[i].zknode);
        struct String_vector strings;
        DEBUG_PRINT("services[%d].zknode:%s", i, services[i].zknode); 
        int ret = zoo_get_children(zkhandle, services[i].zknode, 0, &strings);
        if (ret) {
            ERROR_PRINT("Error %d for %s", ret, "get_children");
            break;
        }

        /* 监视league下的节点变化事件 */
        zkGetLeader( services[i].zknode, strings);
        struct Stat stat;
        DEBUG_PRINT("zoo_wget_children2 services[%d].zknode %s", i , services[i].zknode);
        ret = zoo_wget_children2(zkhandle, services[i].zknode, election_watcher, this, &strings, &stat);
        if (ret) {
            fprintf(stderr, "error %d of %s\n", ret, "wget children2");
            exit(EXIT_FAILURE);
        }
        deallocate_String_vector(&strings);
    }
}

void zkClient::zkGetLeader(std::string path, struct String_vector strings ) {
    DEBUG_PRINT("zkGetLeader begin path %s", path.c_str());
    DEBUG_PRINT("strings.coun %u",strings.count);
    if ( strings.count<=0 ) {
        master_value = "";
        fprintf(stderr, "can not found node_path:%s\n", path.c_str());
        return; 
    }

    int myid = 0;
    for ( int i=1; i<strings.count; i++ ) {
        if ( strcmp(strings.data[myid], strings.data[i])>0 ) {
            myid = i;
        }
    }
    char leader_path[PATH_BUFFER_LEN] = {0};
    sprintf(leader_path, "%s/%s", path.c_str(), strings.data[myid]);
    DEBUG_PRINT("leader_path %s", leader_path);

    char buff[PATH_BUFFER_LEN] = {0};
    int buff_len = PATH_BUFFER_LEN;
    int ret = zoo_get(zkhandle, leader_path, 0, buff, &buff_len, NULL);
    DEBUG_PRINT("zoo_get leader_path %s ret %d", leader_path, ret);
    if (ret) {
        fprintf(stderr, "error %d when get leader from %s", ret, strings.data[myid]);
        master_value = "";
        exit(EXIT_FAILURE);
    }

    DEBUG_PRINT("master node data:%s", buff);
    master_value =  std::string(buff);
}

void election_watcher(zhandle_t * zh, int type, int state,const char* path, void* watcherCtx){
    //watch_func_para_t* para= (watch_func_para_t*)watcherCtx;
    //DEBUG_PRINT("[client] something happened:\n");
    //DEBUG_PRINT("type - %d\n", type);
    //DEBUG_PRINT("state - %d\n", state);
    //DEBUG_PRINT("path - %s\n", path);
    DEBUG_PRINT("election_watcher begin","");

    /* 此处貌似不需要重新监听,但是要重新获得leader */
    struct String_vector strings;  
    struct Stat stat;
    DEBUG_PRINT("zoo_wget_children2 path %s ", path);
    int ret = zoo_wget_children2(zh, path, election_watcher, watcherCtx, &strings, &stat);
    if (ret) {
        printf("error %d of %s\n", ret, "wget children2");
        return;
    }

    zkClient * zkclient = (zkClient*)watcherCtx;
    zkclient->zkGetLeader(path, strings);
    std::string master_value = zkclient->getMasterValue();
    zkclient->func_master_value_onchange(master_value);
    deallocate_String_vector(&strings);

    DEBUG_PRINT("election_watcher end", "");
}

void clients_watcher_g(zhandle_t * zh, int type, int state, const char* path, void* watcherCtx) {
    DEBUG_PRINT("global watcher - type:%d,state:%d", type, state);
    if ( type==ZOO_SESSION_EVENT ) {
        if ( state==ZOO_CONNECTED_STATE ) {
            DEBUG_PRINT("connected to zookeeper service successfully!","");
            DEBUG_PRINT("timeout:%d", zoo_recv_timeout(zh));
        }
        else if ( state==ZOO_EXPIRED_SESSION_STATE ) {
            DEBUG_PRINT("zookeeper session expired!","");
        }
    }
    else {
        DEBUG_PRINT("other type:%d", type);
    }
}


