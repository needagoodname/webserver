#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 线程同步机制封装类

// 互斥锁类
class locker {
public:
    locker();

    ~locker();

    bool lock();

    bool unlock();
    pthread_mutex_t *get();

private:
    pthread_mutex_t m_mutex;
};


// 条件变量类
class cond {
public:
    cond();
    ~cond();

    bool wait(pthread_mutex_t *);
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t);
    bool signal();
    bool broadcast();
private:
    pthread_cond_t m_cond;
};


// 信号量类
class sem {
public:
    sem() ;
    sem( int );
    ~sem();
    // 等待信号量
    bool wait();
    // 增加信号量
    bool post();
private:
    sem_t m_sem;
};

#endif