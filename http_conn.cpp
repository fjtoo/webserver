#include "http_conn.h"

// 将某fd设置为非阻塞
void setnonblocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}

// 在epollfd实例中添加对fd的监听
void epoll_addfd(int epollfd, int fd, bool one_shot) {
    struct epoll_event epev;
    epev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    epev.data.fd = fd;

    // 设置one_shot
    if(one_shot) {
        epev.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &epev);
    // 设置为非阻塞
    setnonblocking(fd);
}

// 在epollfd实例中移除对fd的监听
void epoll_removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    // 注意：直接在这里关闭了fd，因为移除监听后不会再用到fd了
    close(fd); 
}

/* 在epollfd实例中修改对fd的监听事件
注意，重置了socket上的EPOLLONESHOT事件，保证该socket下次可读*/
void epoll_modifyfd(int epollfd, int fd, int ev) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

void http_conn::process() {
    // 处理客户端请求

} 

void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // 设置一下端口复用
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 把该http连接的fd加入到epoll中
    epoll_addfd(m_epollfd, m_sockfd, true);
    ++m_user_count;
}

void http_conn::close_conn() {
    if(m_sockfd != -1) {
        epoll_removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_user_count;
    }
}

bool http_conn::read() {
    // 返回false表示读失败或者读完了
    if(m_read_index >= READ_BUFFER_SIZE) {
        // 说明读缓冲区已经满了
        return false;
    }

    int bytes_read = 0;
    while(1) {
        // 读数据
        bytes_read = recv(m_sockfd, m_read_buf+m_read_index, READ_BUFFER_SIZE-m_read_index, 0);
        if(bytes_read == -1) {
            // 注意，由于m_sockfd是非阻塞的，所以-1不一定表示出错，需要根据errno判断
            // 见游双书P126
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // 因为epoll返回说明肯定有数据要读，这里表示已经读完了。
                break; 
            }
            return false;
        } else if(bytes_read == 0) {
            // 表示对方关闭连接
            return false;
        }
    }
    printf("读到了数据：%s\n", m_read_buf);
    return true;
}

bool http_conn::write() {
    return true;
}