#ifndef THREADPOLL_H
#define THREADPOLL_H

#include<pthread.h>
#include<list>
#include"locker.h"
#include<exception>
#include<iostream>
#include"http_conn.h"

//线程池类
template<class T>
class threadpool{
public:
        threadpool(int thread_number=8,int max_request=10000);
        ~threadpool();
        bool append(T* request);
private: 
        static void*worker(void*);
        void run();

private:
    //线程数量
    int m_pthread_number;

    //线程池数组
    pthread_t* m_pthreads;

    //请求队列
    std::list<T*> m_workqueue;

    //请求队列的允许最大数量
    int m_max_requests;

    //互斥锁
    locker m_queuelocker;

    //信号量判断是否有任务处理
    sem m_queuestat;

    //是否结束线程
    bool m_stop;


};

template<typename T>
threadpool<T>::threadpool(int thread_number,int max_request):m_pthread_number(thread_number), 
    m_max_requests(max_request), m_stop(false), m_pthreads(NULL) {
    if(thread_number<=0||max_request<=0)
        throw std::exception();
    m_pthreads=new pthread_t[thread_number];
    if(!m_pthreads) {
        throw std::exception();
    }
    for(int i=0;i<thread_number;i++){
        std::cout<<"正在创建第"<<i<<"个线程"<<std::endl;
        pthread_create(m_pthreads+i,NULL,worker,this);
        if(pthread_detach(m_pthreads[i])!=0){
            delete []m_pthreads;
            throw std::exception();
        }


    }
}

//此函数和run函数组合为每个进程运行的代码
template<typename T>
void* threadpool<T>::worker(void*arg){
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}


template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if ( m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T*front=m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!front)
            continue;
        front->process();
    }
}

template< typename T >
threadpool< T >::~threadpool() {
    delete [] m_pthreads;
    m_stop = true;
}

template< typename T >
bool threadpool< T >::append(T*request){
    m_queuelocker.lock();
    if ( m_workqueue.size() > m_max_requests ) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}


#endif