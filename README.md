# webserver

## ThreadPool
线程池设计为模板类，包含 `m_threads` 线程数组和 `m_workqueue` 任务队列

线程池的工作原理（体现多线程利用多核并行而不是并发，注意由于os调度仍可能在线程池的环境下多线程单核并发，本demo应该不会出现）是创建多个线程后分离，通过 `worker` 函数共享任务队列（ `pthread_create` 将 `*this` 作为参数传给worker使每个子线程有一个指向主线程threadpool实例的指针），所以需要锁和信号量解决race condition

目前的实现方法不理想，需要频繁用锁和信号量，线程池实例暴露过多

## TimeHeap
定时容器类，沿用了开始时小根堆模拟定时容器的timeheap名，现在通过 stl中容器 `set`（红黑树）实现定时容器，元素为 `pair<time_t(触发时间), p_timer(定时器本身)>` ，pair可以保证元素的唯一性 

提供了修改、添加、触发多个功能，由于 `adjust_timer` 的原因从 `priority_queue` 改为 `set`

## Question
1. g++编译模板类时，声明要和实现放在同一个文件中
2. 试图用stl提供的priority_queue实现小根堆定时器然而无法随机访问，沿用了vector模拟，增加上虑操作优化插入的堆调整
3. 删除沿用从头调整，因为删除影响删除位置以后所有下标（如何优化删除？）
4. 添加日志缓冲池与异步日志
5. 用优先队列重构时间堆，用对象调整取代unordered_map随机访问，无法处理历史连接遗留定时器，考虑用set重构以及如何多线程处理（未加锁）
6. 使用RAII封装定时器指针，在时间堆add和pop时使用move()完成析构定时器对象，修改为优先队列存储weak_ptr，pop时无法升级为shared_ptr，isDeleted=true
7. 优先队列比较函数仿函数和lamda函数失败，问题待查明
8. typedef有作用域
9. 用set重构时间堆，pair->返回值均为拷贝值，用unique_ptr封装定时器无法获得内容，沿用shared_ptr
