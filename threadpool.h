#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "tcpserver.h"
#include <vector>
using std::vector;

// 线程池类，将它定义为模板类是为了代码复用，模板参数T是任务类
template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number, int max_requests);
    ~threadpool(){};
    // bool append(T *request, int);
    bool append(T *);
    bool shut(int);

private:
    // 描述线程池的数组
    vector<tcpserver<T>> m_threads;
    int next;
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : m_threads(thread_number), next(0)
{
    if ((thread_number <= 0) || (max_requests <= 0))
    {
        throw std::exception();
    }
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    // if (!append(request, next))
    // return false;
    if (!m_threads[next].append(request))
    {
        return false;
    }
    next = (next + 1) % m_threads.size();
    return true;
}

// template <typename T>
// bool threadpool<T>::append(T *request, int uid)
// {
//     if (m_threads[uid].append(request))
//     {
//         return true;
//     }
//     return false;
// }

template <typename T>
bool threadpool<T>::shut(int uid)
{
    if(uid >= m_threads.size()) return false;
    m_threads[uid].shut();
    return true;
}

#endif