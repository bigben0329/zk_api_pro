#include "zk_client_api.h"
#include "util.h"

using namespace std;

zkClient::zkClient()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    Timeout = 50000;

}

zkClient::~zkClient()
{
    zookeeper_close(zkhandle);
}

void zkClient::zkLoadConf(const char * filename)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE * fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("open config file %s error!\n", filename);
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (line[0]=='#') // 跳过注释
            continue;
        printf("[%zu]line:%s\n", read, line);
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
}


void zkClient::zkInit()
{
    //load host
    zkHost = config["zkhost"];
    std::string &ss = config["service"];

    std::vector<std::string> slist = split(ss, ',');
    int service_len = slist.size();

    name_service_t * services = (name_service_t *)malloc( service_len * sizeof(name_service_t) );
    for ( int i=0; i<service_len; i++ ) {
        strcpy(services[i].name, slist[i].c_str());
        std::string tmp = config["zkroot"];
        tmp += "/";
        tmp += slist[i];
        strcpy(services[i].zknode, tmp.c_str());
        services[i].hostip = NULL;
        services[i].hostport = 0;
    }

    printf("zkHost:%s\n", zkHost.c_str()); 
    zkhandle = zookeeper_init(zkHost.c_str(), clients_watcher_g, Timeout, 0, NULL, 0);
    if ( zkhandle==NULL ) {
        fprintf(stderr, "error connect to zookeeper service.\n");
        exit(EXIT_FAILURE);
    }

    for ( int i=0; i<service_len; i++ ) {
        printf("[%s]\n", services[i].zknode);
        struct String_vector strings;
        printf("services[%d].zknode:%s\n", i, services[i].zknode); 
        int ret = zoo_get_children(zkhandle, services[i].zknode, 0, &strings);
        if (ret) {
            fprintf(stderr, "Error %d for %s\n", ret, "get_children");
            exit(EXIT_FAILURE);
        }

        zkGetLeader( services[i].zknode, strings);

        /* 监视league下的节点变化事件 */
        struct Stat stat;
        ret = zoo_wget_children2(zkhandle, services[i].zknode, election_watcher, this, &strings, &stat);
        if (ret) {
            fprintf(stderr, "error %d of %s\n", ret, "wget children2");
            exit(EXIT_FAILURE);
        }
    }
}

void zkClient::zkGetLeader(std::string path, struct String_vector strings ) {
    printf("zkGetLeader begin path %s\n", path.c_str());
    printf("strings.coun %u \n",strings.count);
    if ( strings.count<=0 ) {
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
    printf("leader_path %s \n", leader_path);

    char buff[PATH_BUFFER_LEN] = {0};
    int buff_len = PATH_BUFFER_LEN;
    int ret = zoo_get(zkhandle, leader_path, 0, buff, &buff_len, NULL);
    printf("zoo_get leader_path %s ret %d \n", leader_path, ret);
    if (ret) {
        fprintf(stderr, "error %d when get leader from %s\n", ret, strings.data[myid]);
        exit(EXIT_FAILURE);
    }

    printf("master node data:%s\n", buff);
    masterStr =  std::string(buff);
}

void election_watcher(zhandle_t * zh, int type, int state,const char* path, void* watcherCtx){
    watch_func_para_t* para= (watch_func_para_t*)watcherCtx;
    printf("[client] something happened:\n");
    printf("type - %d\n", type);
    printf("state - %d\n", state);
    printf("path - %s\n", path);
    // printf("data - %s\n", para->node);

    /* 此处貌似不需要重新监听,但是要重新获得leader */
    struct String_vector strings;  
    struct Stat stat;
    int ret = zoo_wget_children2(zh, path, election_watcher, watcherCtx, &strings, &stat);
    if (ret) {
        fprintf(stderr, "error %d of %s\n", ret, "wget children2");
        exit(EXIT_FAILURE);
    }
    ((zkClient*)watcherCtx)->zkGetLeader(path, strings);
    deallocate_String_vector(&strings);
}

void clients_watcher_g(zhandle_t * zh, int type, int state, const char* path, void* watcherCtx) {
    printf("global watcher - type:%d,state:%d\n", type, state);
    if ( type==ZOO_SESSION_EVENT ) {
        if ( state==ZOO_CONNECTED_STATE ) {
            printf("connected to zookeeper service successfully!\n");
            printf("timeout:%d\n", zoo_recv_timeout(zh));
        }
        else if ( state==ZOO_EXPIRED_SESSION_STATE ) {
            printf("zookeeper session expired!\n");
        }
    }
    else {
        printf("other type:%d\n", type);
    }
}


