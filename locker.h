#ifndef LOCKER_H
#define LOCKER_H

#include<pthread.h>
#include<exception>
#include<semaphore.h>
// 线程同步机制封装类，方便复用

// 互斥锁类
class locker
{
private:
    pthread_mutex_t m_mutex;
public:
    locker();
    ~locker();
    bool lock(); // 加锁
    bool unlock(); // 解锁
    pthread_mutex_t* get(); // 用于
};

locker::locker()
{
    if(pthread_mutex_init(&m_mutex, NULL) != 0) {
        // 初始化成功返回0,失败非0
        throw std::exception();
    }
}

locker::~locker()
{
    pthread_mutex_destroy(&m_mutex);
}

bool locker::lock()
{
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool locker::unlock()
{
    return pthread_mutex_unlock(&m_mutex) == 0;
}

pthread_mutex_t* locker::get()
{
    return &m_mutex;
}


// 信号量类
class sem
{
private:
    sem_t m_sem;
public:
    sem();
    ~sem();
    bool wait();
    bool post();
};

sem::sem()
{
    if(sem_init(&m_sem, 0, 0) != 0) {
        throw std::exception();
    }
}

sem::~sem()
{
    sem_destroy(&m_sem);
}

bool sem::wait()
{
    return sem_wait(&m_sem);
}

bool sem::post()
{
    return sem_post(&m_sem);
}


// 条件变量类
class cond
{
private:
    pthread_mutex_t m_mutex; // 用于保证条件变量的wait操作的原子性
    pthread_cond_t m_cond;
public:
    cond();
    ~cond();
    bool wait();
    bool signal();
};

cond::cond()
{
    if(pthread_mutex_init(&m_mutex, NULL) != 0) {
        throw std::exception();
    }
    if(pthread_cond_init(&m_cond, NULL) != 0) {
        pthread_mutex_destroy(&m_mutex);
        throw std::exception();
    }
}

cond::~cond()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

bool cond::wait()
{
    int ret = 0;
    pthread_mutex_lock(&m_mutex);
    ret = pthread_cond_wait(&m_cond, &m_mutex);
    pthread_mutex_unlock(&m_mutex);
    return ret == 0;
}

bool cond::signal()
{
    return pthread_cond_signal(&m_cond) == 0;
}

#endif