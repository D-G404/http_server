#ifndef LOCKER_H
#define LOCKER_H
#include<pthread.h>
#include<exception>
#include<semaphore.h>

//线程同步封装  

//互斥锁类
class locker{
public:
    
    locker(); 
    ~locker();
    bool lock();
    bool unlock();
    pthread_mutex_t* get();

private:
    pthread_mutex_t m_mutex;
};

//条件变量类
class cond{
public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t* mutex);
    bool timedwait(pthread_mutex_t*mutex,timespec t);
    bool signal();
    bool broadcast();
private:
    pthread_cond_t m_cond;
};


class sem{
public:
    sem();
    sem(int num);
    ~sem();
    bool wait();  
    bool post();

private:
    sem_t m_sem;
};



#endif