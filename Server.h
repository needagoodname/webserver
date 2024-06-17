#ifndef SERVER_H
#define SERVER_H

#include <signal.h>
#include "threadpool.h"
#include "timeheap.h"
#include "http_conn.h"
#include "bloomFilter.h"
#include "IpTree.h"
#include "log.h"

static const int MAX_FD = 65536;   // 最大的文件描述符个数
static const int MAX_EVENT_NUMBER = 10000;  // 监听的最大的事件数量
static const int UDP_BUFFER_SIZE = 1024;
static const int TCP_BUFFER_SIZE = 512;

extern void addfd( int epollfd, int fd, bool one_shot );
extern void removefd( int epollfd, int fd );
extern int setnonblocking( int fd );

class Server {
public:
    Server(const int& p);
    ~Server();

    int run();

    void sig_handler(int sig);
    void addsig(int sig);
    void time_handler( time_heap* m_time);
    void cb_func( int epollfd, int sockfd );

private:
    int pipefd[2];
    int port;
    int epollfd;
    int listenfd;

    http_conn *users;
    epoll_event *events;

    struct sockaddr_in address;
};

#endif