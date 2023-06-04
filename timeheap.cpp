#include "timeheap.h"

heap_timer::heap_timer( int delay )
{
    expire = time( NULL ) + delay;
}

void time_heap::adjust_timer()
{

    time_t cur = time( NULL );
    cur = cur + 3 * TIMESLOT;
    p_timer t = std::move(array.begin()->second);
    t->expire = cur;
    add_timer( t );
    pop_timer();

}

time_heap::time_heap( int fd, http_conn* users ) : epollfd( fd ), user(users) {}

time_heap::~time_heap()
{
    while( !empty() )
    {
        pop_timer();
    }
}

void time_heap::add_timer( const p_timer& timer )
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        array.emplace( std::make_pair(timer->expire, timer) );
    }
}

void time_heap::pop_timer()
{
    if( empty() ) return;
    {
        std::lock_guard<std::mutex> locker(mtx_);
        p_timer t = std::move(array.begin()->second);
        array.erase(array.begin());
    }
}

void time_heap::tick()
{
    time_t cur = time( NULL );
    while( !empty() )
    {
        auto tmp = array.begin();
        if( tmp->first > cur ) {
            return;
        }

        std::lock_guard<std::mutex> locker(mtx_);
        if( user[tmp->second->sockfd].getfd() == -1 ) {
            pop_timer();
            continue;
        }
        if( user[tmp->second->sockfd].last_active <= tmp->first )
        {
            tmp->second->cb_func( epollfd, tmp->second->sockfd );
        } 
        else adjust_timer();
        pop_timer();
    }
}

bool time_heap::empty() const
{
    return array.empty();
}