#include "timewheel.h"

tw_timer::tw_timer( int rot, int ts )
: next(nullptr), prev(nullptr), rotation( rot ), time_slot( ts ) {}

time_wheel::time_wheel() : cur_slot(0)
{
    for( int i = 0; i < N; ++i ) slots[i] = NULL;// 初始化每个槽头结点
}

time_wheel::~time_wheel()
{
    for( int i = 0; i < N; ++i )
    {
        tw_timer* tmp = slots[i];
        while ( tmp )
        {
            slots[i] = tmp->next;
            delete tmp;
            tmp = slots[i];
        }
        
    }
}

// 根据时间创建定时器并插入槽中
tw_timer* time_wheel::add_timer( int timeout )
{
    if( timeout < 0 ) return NULL;
    int ticks = 0;
    
    // 计算转动了多少个槽后触发，不满1补足为1，其余向下取整
    if( timeout < SI ) ticks = 1;
    else ticks = timeout / SI;

    int rotation = ticks / N;
    int ts = ( cur_slot + ( ticks % N ) ) % N;
    tw_timer* timer = new tw_timer( rotation, ts );

    if( !slots[ts] )
    {
        printf( "add timer, rotation is %d", rotation);
        slots[ts] = timer;
    } else {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }

    return timer;
}

void time_wheel::del_timer( tw_timer* timer )
{
    if( !timer ) return;

    int ts = timer->time_slot;

    if( timer == slots[ts] ) 
    {
        slots[ts] = slots[ts]->next;
        if( slots[ts] ) slots[ts]->prev = nullptr;
        delete timer;
    } else {
        timer->prev->next = timer->next;
        if( timer->next ) timer->next->prev = timer->prev;
        delete timer;
    }
}

void time_wheel::tick()
{
    tw_timer* tmp = slots[cur_slot];
    printf( "current slot is %d\n", cur_slot);
    while( tmp )
    {
        if( tmp->rotation > 0 )
        {
            --tmp->rotation;
            tmp = tmp->next;
        } else {
            tmp->cb_func( tmp->user_data );
            if( tmp == slots[cur_slot] )
            {
                del_timer(tmp);
            } else {
                tw_timer* t = tmp->next;
                del_timer(tmp);
                tmp = t;
            }
        }
    }
    cur_slot = ++cur_slot % N;
}