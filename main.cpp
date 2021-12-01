#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <libgen.h>
#include <signal.h>
#include "threadpool.h"
#include "http_conn.h"
#include "locker.h"

#define MAX_FD 65535 // 最大的客户端连接数
#define MAX_EVENT_NUMBER 10000 // epoll最大监听数量

// 在epollfd实例中添加对fd的监听
extern void epoll_addfd(int epollfd, int fd, bool one_shot);
// 在epollfd实例中移除对fd的监听
extern void epoll_removefd(int epollfd, int fd);
/* 在epollfd实例中修改对fd的监听事件
注意，重置了socket上的EPOLLONESHOT事件，保证该socket下次可读*/
extern void epoll_modifyfd(int epollfd, int fd, int ev);

// 信号处理函数，第一个参数是信号名，第二个是处理函数
void addsig(int sig, void(handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}


int main(int argc, char* argv[]) {
    // 使用命令行参数传入端口号
    if(argc <= 1) {
        printf("usage: %s port_number\n", basename(argv[0]));
        exit(-1);
    }
    
    // 忽略SIGPIPE信号
    addsig(SIGPIPE, SIG_IGN);

    // 初始化线程池
    threadpool<http_conn>* pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    } catch(...) {
        exit(-1);
    }

    // 创建一个数组，存储所有客户端信息，方便后续操作
    http_conn* users = new http_conn[MAX_FD];

    // 创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 设置端口复用，以免端口处于TIME_WAIT状态而无法使用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = 0;
    // 绑定IP与端口号
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));
    bind(listenfd, (struct sockaddr*)&address, sizeof(address));

    // 监听
    listen(listenfd, 5);

    // 创建epoll对象
    int epollfd = epoll_create(666);

    // 添加对listenfd的监听
    epoll_addfd(epollfd, listenfd, false);
    // 然后赋给http_conn的静态成员
    http_conn::m_epollfd = epollfd;

    epoll_event events[MAX_EVENT_NUMBER];
    while(1) {
        // 阻塞地等待epoll上注册的事件发生
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(num < 0 && errno != EINTR) {
            printf("epoll fail");
            break;
        }

        for(int i = 0; i < num; ++i) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                // 监听到listenfd上的连接请求时
                struct sockaddr_in client_address;
                socklen_t client_arrdlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_arrdlen);

                if(http_conn::m_user_count >= MAX_FD) {
                    // 连接的客户端数到达上限时
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd, client_address);
            } else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 对方异常断开或者错误等事件
                users[sockfd].close_conn();
            } else if(events[i].events & EPOLLIN) {
                // 当检测到某fd上的读事件
                if(users[sockfd].read()) {
                    // 一次性把内容读完，存到http_conn.m_read_buf里
                    // 然后把该http_conn交给工作线程处理
                    pool->append(&users[sockfd]);
                } else {
                    // 读失败或者读完了
                    users[sockfd].close_conn();
                } 
            } else if(events[i].events & EPOLLOUT) {
                // 检测到写事件
                if(!users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(listenfd);
    close(epollfd);
    delete[] users;
    delete pool;

    return 0;
}