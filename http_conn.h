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
    int m_socket; // 该HTTP连接的套接字
    sockaddr_in m_address; // 通信地址
public:

    static int m_epollfd; // 所有socket事件都注册到这一个epoll实例上
    static int m_user_count; // 统计用户数量

    http_conn();
    void process(); // 处理客户端请求
    void init(int sockfd, const sockaddr_in& addr); // 初始化新接受的连接
    ~http_conn();
};

http_conn::http_conn()
{
}

http_conn::~http_conn()
{
}


#endif