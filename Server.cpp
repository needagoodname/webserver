#include "Server.h"

//信号处理函数，统一事件源
void Server::sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send( pipefd[1], (char*)&msg , 1, 0 );
    errno = save_errno;
}

void Server::addsig(int sig) {
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sig_handler;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

// 处理定时事件
void Server::time_handler( time_heap* m_time)
{
    m_time->tick();
    alarm( TIMESLOT );
}

// 定时器回调函数，删除并关闭非活动连接socket上的注册事件
void Server::cb_func( int epollfd, int sockfd )
{
    cout<<"timecl"<<endl;
    users[sockfd].close_conn();
    close( sockfd );
}

Server::Server(const int& p):port(p) {
    http_conn* users = new http_conn[ MAX_FD ];
    addsig( SIGPIPE );
    threadpool< http_conn > pool( 8, 100 );
    listenfd = socket( PF_INET, SOCK_STREAM, 0 );

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons( port );

    // 创建epoll对象，和事件数组
    epoll_event *events = new epoll_event[MAX_EVENT_NUMBER];
    epollfd = epoll_create( 5 );
}

int Server::run() {

    int ret = 0;

    // 端口复用
    int reuse = 1;
    if(setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) ) == -1 )
    {
        return -1;
    }
    
    if( ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) ))
    {
        printf( "tbind errno is %d\n", errno );
        return -1;
    } 
    else {
        ret = listen( listenfd, 5 );
        assert( ret != -1 );
    }

    // 添加到epoll对象中
    addfd( epollfd, listenfd, false );
    http_conn::m_epollfd = epollfd;

    time_heap m_time( epollfd, users );

    ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pipefd );
    setnonblocking( pipefd[1] );
    addfd( epollfd, pipefd[0], false );

    bool stop_server = false;
    bool timeout = false;
    alarm( TIMESLOT * 10 );

    while( !stop_server ) {
        
        int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );

        if ( ( number < 0 ) && ( errno != EINTR ) ) {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; ++i ) {
            
            int sockfd = events[i].data.fd;
            // printf( "position1\n" );
            
            if( sockfd == listenfd ) {
                
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                cout<<client_address.sin_addr.s_addr<<endl;
                // printf( "connfd : %d\n", connfd);

                if ( connfd < 0 ) {
                    printf( "errno is: %d\n", errno );
                    continue;
                }

                if( http_conn::m_user_count >= MAX_FD ) {
                    close(connfd);
                    continue;
                }
                
                time_t cur = time( NULL );
                // time_heap::p_timer t = std::make_shared<heap_timer>(3 * TIMESLOT);
                // t->sockfd = connfd;
                // t->cb_func = cb_func;
                // printf( "position3\n" );
                m_time.add_timer( connfd, cb_func, 3 * TIMESLOT );
                // printf( "position4\n" );
                users[connfd].init( connfd, client_address, cur );
            } 
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ) {
                users[sockfd].close_conn();

            }
            else if( events[i].events & EPOLLIN ) {

                if( sockfd == pipefd[0] ) {
                    int sig;
                    char signals[1024];
                    ret = recv( pipefd[0], signals, sizeof(signals), 0 );
                    if( ret == -1 || ret == 0 ) continue;
                    else {
                        for( int i = 0; i < ret; ++i ) {
                            switch( signals[i] ) {
                                case SIGCHLD:
                                case SIGHUP:
                                {
                                    continue;
                                }
                                case SIGTERM:
                                case SIGINT:
                                {
                                    stop_server = true;
                                }
                                case SIGALRM:
                                {
                                    timeout = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if( users[sockfd].readFd() ) {

                    pool.append( users + sockfd );
                    users[sockfd].last_active = time(NULL);

                } else {
                    users[sockfd].close_conn();

                }

            } 
            else if( events[i].events & EPOLLOUT ) {

                if( !users[sockfd].writeFd() ) {
                    users[sockfd].close_conn();
                }
                users[sockfd].last_active = time(NULL);
                
            }
        }

        if( timeout )
        {
            time_handler( &m_time );
            timeout = false;
        }
    }
    
}

Server::~Server() {
    close( epollfd );
    close( listenfd );
    delete [] users;
    delete [] events;
}