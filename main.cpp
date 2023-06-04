#include "threadpool.h"
#include "timeheap.h"
// #include "log.h"

static const int MAX_FD = 65536;   // 最大的文件描述符个数
static const int MAX_EVENT_NUMBER = 10000;  // 监听的最大的事件数量
static const int UDP_BUFFER_SIZE = 1024;
static const int TCP_BUFFER_SIZE = 512;
static int pipefd[2];

extern void addfd( int epollfd, int fd, bool one_shot );
extern void removefd( int epollfd, int fd );
extern int setnonblocking( int fd );

http_conn* users = new http_conn[ MAX_FD ];

//信号处理函数，统一事件源
void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send( pipefd[1], (char*)&msg , 1, 0 );
    errno = save_errno;
}

void addsig(int sig) {
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sig_handler;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

// 处理定时事件
void time_handler( time_heap* m_time)
{
    m_time->tick();
    alarm( TIMESLOT );
}

// 定时器回调函数，删除并关闭非活动连接socket上的注册事件
void cb_func( int epollfd, int sockfd )
{
    users[sockfd].close_conn();
    close( sockfd );
}

int main( int argc, char* argv[] ) {
    
    if( argc <= 1 ) {
        printf( "usage: %s port_number\n", basename( argv[0] ) );
        return 1;
    }

    // Log logDoc()

    int port = atoi( argv[1] );
    addsig( SIGPIPE );

    threadpool< http_conn >* pool = nullptr;
    try {
        pool = new threadpool<http_conn>;
    } catch( ... ) {
        return 1;
    }

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );

    int ret = 0;
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons( port );

    // 端口复用
    int reuse = 1;
    if(setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) ) == -1 )
    {
        return -1;
    }
    
    if( ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) ))
    {
        printf( "tbind errno is %d\n", errno );
        return 1;
    } 
    else {
        ret = listen( listenfd, 5 );
        assert( ret != -1 );
    }

    // 创建epoll对象，和事件数组
    epoll_event events[ MAX_EVENT_NUMBER ];
    int epollfd = epoll_create( 5 );

    // 添加到epoll对象中
    addfd( epollfd, listenfd, false );
    http_conn::m_epollfd = epollfd;

    time_heap* m_time = nullptr;
    try{
        m_time = new time_heap( epollfd, users );
    } catch( ... ) {
        return 1;
    }

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
            
            if( sockfd == listenfd ) {
                
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                
                if ( connfd < 0 ) {
                    printf( "errno is: %d\n", errno );
                    continue;
                }

                if( http_conn::m_user_count >= MAX_FD ) {
                    close(connfd);
                    continue;
                }
                
                time_t cur = time( NULL );
                time_heap::p_timer t = std::make_shared<heap_timer>(3 * TIMESLOT);
                t->sockfd = connfd;
                t->cb_func = cb_func;
                m_time->add_timer( t );
                users[connfd].init( connfd, client_address, cur );

            } 
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ) {
                
                users[sockfd].close_conn();
                // users[sockfd] = nullptr;

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
                else if( users[sockfd].read() ) {

                    pool->append( users + sockfd );
                    users[sockfd].last_active = time(NULL);

                } else {

                    users[sockfd].close_conn();
                    // users[sockfd] = NULL;

                }

            } 
            else if( events[i].events & EPOLLOUT ) {

                if( !users[sockfd].write() ) {

                    users[sockfd].close_conn();
                    // users[sockfd] = NULL;
                
                }
                users[sockfd].last_active = time(NULL);
            }
        }

        if( timeout )
        {
            time_handler( m_time );
            timeout = false;
        }
    }
    
    close( epollfd );
    close( listenfd );
    delete [] users;
    delete pool;
    return 0;
}