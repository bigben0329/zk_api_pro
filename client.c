#include "zk_client_api.h"

void run() {
    printf("client is running on\n");
}

int main(int argc, const char * argv[]) {

    zkClient zkclient;
    zkclient.zkLoadConf("./conf/zk.conf");
    zkclient.zkInit();
    printf("master str %s \n",zkclient.getMasterStr().c_str());

    int total = 20;
    while (total-- > 0) {
        run();
        sleep(1);
    }
    return 0;
}
