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
#include <errno.h>

class http_conn
{
public:
    static const int READ_BUFFER_SIZE = 2048;  // 常量 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024; // 常量 写缓冲区的大小

private:
    int m_sockfd; // 该HTTP连接的套接字，如果设置为-1表示已经关闭
    sockaddr_in m_address; // 通信地址
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_index; // 表示当前读缓冲区中读入的数据最后一个字节在读缓冲区中的位置

public:

    static int m_epollfd; // 所有socket事件都注册到这一个epoll实例上
    static int m_user_count; // 统计用户数量(注意静态成员必须在类外进行定义，这里只是声明)

    void process(); // 处理客户端请求
    void init(int sockfd, const sockaddr_in& addr); // 初始化新接受的连接
    void close_conn(); // 关闭连接
    bool read();       // 非阻塞读
    bool write();      // 非阻塞写
};



#endif