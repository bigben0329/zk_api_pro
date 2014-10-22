#include "zk_server_api.h"
#include "util.h"

#define PATH_BUFFER_LEN 64

typedef struct _watch_func_para_t {
        zhandle_t *zkhandle;
            char node[PATH_BUFFER_LEN];
} watch_func_para_t; 

void service_watcher_g(zhandle_t * zh, int type, int state, const char* path, void* watcherCtx) {
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

void election_watcher(zhandle_t * zh, int type, int state,
        const char* path, void* watcherCtx) {
    watch_func_para_t* para= (watch_func_para_t*)watcherCtx;
    printf("something happened:\n");
    printf("type - %d\n", type);
    printf("state - %d\n", state);
    printf("path - %s\n", path);
    printf("data - %s\n", para->node);
}

static void get_node_name(const char *buf, char *node) {                                                                                
    const char *p = buf;
    int i;
    for ( i=strlen(buf); i>=0; i-- ) {
        if (*(p + i) == '/') {
            break;
        }
    }
    strcpy(node, p + i + 1);
    return;
}

zkServer::zkServer(std::string name):serName(name)
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    Timeout = 30000;
    serName = "/league";
    zkNodePath = "/svr";
    zkRootPath = "/app/hd/service";
}

zkServer::zkServer()
{
    zkServer("default");
}

zkServer::~zkServer()
{
    if(zkhandle != NULL)
    {
        //zookeeper_close(zkhandle);
    }
};

void zkServer::zkLoadConf(const char * filename)
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


int zkServer::is_leader() 
{
    int ret = 0;
    int flag = 1;

    struct String_vector strings;
    ret = zoo_get_children(zkhandle, (zkRootPath+serName).c_str(), 0, &strings);
    if (ret) {
        fprintf(stderr, "Error %d for %s\n", ret, "get_children");
        exit(EXIT_FAILURE);
    }

    int i;
    for ( i=0; i<strings.count; i++ ) {
        printf("zkNode %s string[%d].data %s \n", zkNode.c_str(), i, strings.data[i]);
        if ( strcmp(zkNode.c_str(), strings.data[i])>0 ) { /* 如果我自己不是最小的节点 */
            flag = 0;
            break;
        }
    }

    return flag;
}


int zkServer::zkInit()
{
    zkHost = config["zkhost"];
    serName = "/"+config["service"];
    zkRootPath = config["zkroot"];
    zkNodePath = config["prefix"];

    int ret = 0;
    zkhandle = zookeeper_init(zkHost.c_str(), service_watcher_g, Timeout, 0, NULL, 0);
    if ( zkhandle==NULL ) {
        fprintf(stderr, "error connect to zookeeper service.\n");
        exit(EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    return ret;
}

int zkServer::zkCreate(const char *data, char *path_buffer, int path_buffer_len)
{
    int ret = 0;
    printf("zkRootPath:%s serName:%s zkNodePath:%s \n", zkRootPath.c_str(), serName.c_str(), zkNodePath.c_str());
    ret = zoo_create(zkhandle, std::string(zkRootPath + serName + zkNodePath).c_str(), data, strlen(data), 
            &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_EPHEMERAL|ZOO_SEQUENCE, 
            path_buffer, 
            path_buffer_len);

    if (ret) {
        fprintf(stderr, "error %d of %s\n", ret, "create node");
        exit(EXIT_FAILURE);
    }
    else {
        char node[PATH_BUFFER_LEN] = {0};
        get_node_name(path_buffer, node);
        printf("path_buffer:%s, node_name:%s\n", path_buffer, node);
        
        /* get data from znode */
        char buff[PATH_BUFFER_LEN] = {0};
        int buff_len = PATH_BUFFER_LEN;
        ret = zoo_get(zkhandle, path_buffer, 0, buff, &buff_len, NULL);
        zkNode = std::string(node);
        printf("node data:%s zkNode:%s \n", buff, zkNode.c_str());
    }

    return ret;
}

/* 监视league下的节点变化事件 */
int zkServer::watchChildren()
{
    int ret = 0;
    watch_func_para_t para;
    memset(&para, 0, sizeof(para));
    para.zkhandle = zkhandle;
    strcpy(para.node, zkNode.c_str());

    struct String_vector strings;  
    struct Stat stat;
    ret = zoo_wget_children2(zkhandle, (zkRootPath+serName).c_str(), election_watcher, &para, &strings, &stat);
    if (ret) {
        fprintf(stderr, "error %d of %s\n", ret, "wget children2");
        exit(EXIT_FAILURE);
    }

    return ret;
}

void zkServer::set_name(const std::string name)
{
    serName = name;
}

void zkServer::set_host(const std::string host)
{
    zkHost = host;
}



