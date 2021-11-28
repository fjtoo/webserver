#ifndef HTTP_CONNNECTION_H
#define HTTP_CONNNECTION_H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>

class http_conn
{
private:
    
public:
    http_conn();
    void process(); // 处理客户端请求
    ~http_conn();
};

http_conn::http_conn()
{
}

http_conn::~http_conn()
{
}


#endif