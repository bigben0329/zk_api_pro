#ifndef _ZK_H_
#define _ZK_H_
/*
 * Filename: zkdef.h
 * Description: 
 * Author: - 
 * Created: 2014-09-17 00:30:56
 * Revision: $Id$
 */ 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper_log.h>


#include "util.h"
#define PATH_BUFFER_LEN 64

char * zkhost;
int pid = 0;


#endif

