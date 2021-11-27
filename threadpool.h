#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<list>
#include<locker.h>
#include<exception>
#include<cstdio>
#include<locker.h>

// 线程池类，定义成模板类
template<typename T>
class threadpool {
private:
    // 线程数量
    int m_thread_number;
    // 线程池数组，大小为线程数量
    pthread_t* m_threads;
    // 请求队列中最大请求数量
    int m_max_requests;
    // 请求队列，用list存储
    std::list<T*> m_workqueue;
    // 互斥锁，用于保护请求队列
    locker m_queuelocker;
    // 信号量，用于判断是否有任务需要处理
    sem m_queuestat;
    // 是否结束线程的判断位
    bool m_stop;

private:
    /*  线程的工作函数
    注意 ：pthread_create的第三个参数，C++里必须是静态函数*/
    static void* worker(void* arg);
    void run();

public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    // 往请求队列中添加任务
    bool append(T* request);

};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : 
    m_thread_number(thread_number), m_max_request(max_requests),
    m_stop(false), m_threads(NULL) {

    if(thread_number <= 0 || max_requests <= 0) {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) {
        throw std::exception();
    }

    // 创建thread_number个线程，并设置线程脱离，便于释放资源
    for(int i = 0; i < m_thread_number; ++i) {
        printf("create the %dth thread\n", i);
        // 这里有个技巧，把this当作参数传给worker，这样能够在worker里调用非静态成员
        if(pthread_create(m_threads + i, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        // 设置线程脱离，线程结束自动释放资源
        if(pthread_detach(m_threads[i]) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock(); // 先加锁，防止两个线程同时通过判断
    if(m_workqueue.size() < m_max_requests) {
        // 把当前请求放入队列
        m_workqueue.push_back(request);
        // 信号量+1以便后续处理
        m_queuestat.post(); 
        m_queuelocker.unlock();
        return true;
    }
    m_queuelocker.unlock();
    return false;
}

template<typename T>
void* threadpool<T>::worker(void* arg) {
    // this是threadpool*类型的
    threadpool* pool = (threadpool*) arg;
    // 运行线程池
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    // 不断从队列里取一个任务然后执行
    while(!m_stop) {
        // 信号量-1
        m_queuestat.wait();
        m_queuelocker.lock();
        // 取第一个任务
        T* request = m_workqueue.front();
        m_workqueue.pop_back();
        m_queuelocker.unlock();
        
        if(!request) {
            continue;
        }
        // 执行这个任务的业务，该线程池用的时候T这个类型必须定义process()
        request->process();
    }
}

#endif