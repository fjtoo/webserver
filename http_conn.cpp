#include "http_conn.h"

// 在epollfd实例中添加对fd的监听
void addfd(int epollfd, int fd, bool one_shot) {
    struct epoll_event epev;
    epev.events = EPOLLIN | EPOLLRDHUP;
    epev.data.fd = fd;

    // 设置one_shot
    if(one_shot) {
        epev.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epev);
}

// 在epollfd实例中移除对fd的监听
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/* 在epollfd实例中修改对fd的监听事件
注意，重置了socket上的EPOLLONESHOT事件，保证该socket下次可读*/
void modifyfd(int epollfd, int fd, int ev) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_socket = sockfd;
    m_address = addr;
}