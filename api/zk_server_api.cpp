#include "zk_server_api.h"
#include "util.h"

#define PATH_BUFFER_LEN 64
#define DEBUG_PRINT(format, ...) printf( "#DEBUG %s %s "format"\n", __FILE__, __FUNCTION__, __VA_ARGS__);
#define ERROR_PRINT(format, ...) \
{ \
printf( "#ERROR %s %s "format"\n", __FILE__, __FUNCTION__, __VA_ARGS__); \
snprintf(error_msg, sizeof(error_msg), "#ERROR %s %s "format"\n", __FILE__, __FUNCTION__, __VA_ARGS__); \
}

typedef struct _watch_func_para_t {
        zhandle_t *zkhandle;
        char node[PATH_BUFFER_LEN];
        zkServer *zks;
         
} watch_func_para_t; 
watch_func_para_t para;

void service_watcher_g(zhandle_t * zh, int type, int state, const char* path, void* watcherCtx) {
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

void election_watcher(zhandle_t * zh, 
    int type, 
    int state,
    const char* path, void* watcherCtx) 
{
    watch_func_para_t* para= (watch_func_para_t*)watcherCtx;
    DEBUG_PRINT("something happened:","");
    DEBUG_PRINT("type - %d", type);
    DEBUG_PRINT("state - %d", state);
    DEBUG_PRINT("path - %s", path);
    DEBUG_PRINT("data - %s", para->node);

    struct String_vector strings;  
    struct Stat stat;
    int ret = zoo_wget_children2(zh, path, election_watcher, watcherCtx, &strings, &stat);
    if(ret) { printf("zoo_wget_children2 path %s error ret %d\n", path, ret);}
    deallocate_String_vector(&strings);

/*
    if(para->zks->isLeader())
    {
        printf("I'm leader now!\n");
    }
    else
    {
        printf("I'm slave.\n");
    }
*/
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

zkServer::zkServer(std::string name):server_name(name)
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    prefix = "/svr";
    root = "/app/ncdz/service";
}

zkServer::zkServer()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    timeout = 30000;
}

zkServer::~zkServer()
{
    if(zkhandle != NULL)
    {
        zookeeper_close(zkhandle);
    }
};

int zkServer::zkLoadConf(const char * filename)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE * fp = fopen(filename, "r");
    if (fp == NULL)
    {
        DEBUG_PRINT("open config file %s error!", filename);
        return -1;
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

    host = config["host"];
    server_name = config["service"];
    root = config["zkroot"];
    prefix = config["prefix"];
    timeout = atoll(config["timeout"].c_str());

    if(host.empty()) host = "0.0.0.0:1234";
    DEBUG_PRINT("host %s", host.c_str());
    if(server_name.empty()) server_name = "default";
    DEBUG_PRINT("server_name %s", server_name.c_str());
    if(root.empty()) root = "root";
    DEBUG_PRINT("root %s", root.c_str());
    if(prefix.empty()) prefix = "prefix";
    DEBUG_PRINT("prefix %s", prefix.c_str());
    if(0 == timeout) timeout = 3000;
    DEBUG_PRINT("timeout %u", timeout);
    


    return 0;
}


int zkServer::isLeader() 
{
    int ret = 0;
    int flag = 1;
    if(NULL == zkhandle)
    {
        ERROR_PRINT(" zkhandle is NULL ", "");
        return -1;
    }

    struct String_vector strings;
    DEBUG_PRINT("root %s server_name %s count %u", root.c_str(), server_name.c_str(), strings.count);
    ret = zoo_get_children(zkhandle, (root+server_name).c_str(), 0, &strings);
    if (ret) {
        ERROR_PRINT("Error %d for %s", ret, "get_children");
        return -1;
    }

    int i;
    for ( i=0; i<strings.count; i++ ) {
        DEBUG_PRINT("node_name %s string[%d].data %s", node_name.c_str(), i, strings.data[i]);
        if ( strcmp(node_name.c_str(), strings.data[i])>0 ) { /* 如果我自己不是最小的节点 */
            flag = 0;
            break;
        }
    }
    deallocate_String_vector(&strings);
    return flag;
}


int zkServer::zkInit()
{
    int ret = 0;
     
    DEBUG_PRINT("host %s timeout %u", host.c_str(), timeout);
    zkhandle = zookeeper_init(host.c_str(), service_watcher_g, timeout, 0, NULL, 0);
    if ( zkhandle==NULL ) {
        ERROR_PRINT("error connect to zookeeper service.", "");
        return EXIT_FAILURE;
    }

    return ret;
}

int zkServer::zkCreate(const char *data, char *path_buffer, int path_buffer_len)
{
    int ret = 0;
    if(NULL == zkhandle)
    {
        ERROR_PRINT(" zkhandle is NULL ", "");
        return -1;
    }

    DEBUG_PRINT("root:%s server_name:%s prefix:%s ", root.c_str(), server_name.c_str(),prefix.c_str());
    ret = zoo_create(zkhandle, std::string(root + server_name + prefix).c_str(), data, strlen(data), 
            &ZOO_OPEN_ACL_UNSAFE, 
            ZOO_EPHEMERAL|ZOO_SEQUENCE, 
            path_buffer, 
            path_buffer_len);

    if (ret) {
        ERROR_PRINT(" error %d of %s ", ret, "create node");
	return -2;
    }
    else 
    {
        char node[PATH_BUFFER_LEN] = {0};
        get_node_name(path_buffer, node);
        node_name = std::string(node);
        DEBUG_PRINT("path_buffer:%s, node_name:%s", path_buffer, node_name.c_str());
        
        /* get data from znode */
        char node_value[PATH_BUFFER_LEN] = {0};
        int value_len = PATH_BUFFER_LEN;
        ret = zoo_get(zkhandle, path_buffer, 0, node_value, &value_len, NULL);
        DEBUG_PRINT("node data:%s node_name:%s", node_value, node_name.c_str());
    }

    return ret;
}

/* 监视league下的节点变化事件 */
int zkServer::watchChildren()
{
    if(NULL == zkhandle)
    {
        ERROR_PRINT(" zkhandle is NULL ", "");
	return -1;
    }

    int ret = 0;
    memset(&para, 0, sizeof(para));
    para.zkhandle = zkhandle;
    strcpy(para.node, node_name.c_str());
    DEBUG_PRINT("node_name %s para.node %s", node_name.c_str(), para.node);
    para.zks = this;

    struct String_vector strings;  
    struct Stat stat;
    ret = zoo_wget_children2(zkhandle, (root+server_name).c_str(), election_watcher, &para, &strings, &stat);
    if (ret) {
        ERROR_PRINT("error %d of %s", ret, "wget children2");
	    return -2;
    }
    deallocate_String_vector(&strings);

    return ret;
}

void zkServer::setName(const std::string name)
{
    server_name = name;
}

void zkServer::setHost(const std::string h)
{
    host = host;
}



