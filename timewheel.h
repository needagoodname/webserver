#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64

class tw_timer;

// 绑定定时器和socket
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    tw_timer* timer;
};

class tw_timer
{
public:
    tw_timer( int, int );

    int rotation;// 记录轮转多少圈后生效
    int time_slot;// 槽序号
    void (*cb_func) (client_data*);// 定时器回调函数
    client_data* user_data;// 客户数据
    tw_timer* next;// 下一个定时器
    tw_timer* prev;// 上一个定时器
};

class time_wheel
{
public:
    time_wheel();
    ~time_wheel();
    tw_timer* add_timer( int );
    void del_timer ( tw_timer* );
    void tick();
private:
    static const int N = 60;// 时间轮上槽的数目
    static const int SI = 1;// 槽间隔1s
    tw_timer* slots[N];
    int cur_slot;
};

#endif