#ifndef MIN_HEAP
#define MIN_HEAP

#include <netinet/in.h>
#include <time.h>
#include <exception>
#include <vector>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include "http_conn.h"

static int BUFFER_SIZE = 64;
static int TIMESLOT = 5;
using std::pair;

// 定时器检测客户端连接状态，触发定时器断开连接
class heap_timer
{
public:
    heap_timer( int );
    void ( *cb_func ) ( int, int ); //callback function
    time_t expire; // 检测时间
    int sockfd = 0; // http连接

};

// 时间器管理
class time_heap
{   
public:

    typedef std::shared_ptr<heap_timer> p_timer;
    time_heap( int, http_conn* );
    ~time_heap();
    void adjust_timer();
    void add_timer( const int& , void(*cb_func)(int, int), const int& );
    void tick();
    bool empty() const;

private:
    struct cmp {
        bool operator() (pair<time_t, p_timer> a, pair<time_t, p_timer> b) {
            return a.first > b.first;
        }
    };
    int epollfd;
    std::mutex mtx_;
    http_conn* user;
    std::set<std::pair<time_t, p_timer>, cmp> array;
    void pop_timer();
    void add_timer( p_timer );
};

#endif