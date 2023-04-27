#include<iostream>
#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/epoll.h>
#include"locker.h"
#include"threadpoll.h"
#include<signal.h>
#include"http_conn.h"
#define MAX_FD 65535
#define MAX_EVENT_NUMBER 1000

//信号处理
void addsig(int sig,void(handler)(int)){
    struct sigaction sa;
    bzero(&sa,sizeof(sa));
    sa.sa_handler=handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig,&sa,NULL);
}

//添加文件描述符到epoll
extern void addfd(int epollfd,int fd,bool one_shot);
//从epoll删除文件描述符
extern void removefd(int epollfd,int fd);
//修改
extern void modfd(int epollfd,int fd,int ev);

int main(int argc,char*argv[]){
    if(argc<=1){
        std::cout<<"按照如下格式运行:"<<basename(argv[0])<<" portname"; //basename 把路径取出来
        exit(-1);
    }
    int port =atoi(argv[1]); //argv[1]为端口号

    //对sigpipe处理  
    addsig(SIGPIPE,SIG_IGN);
    threadpool<http_conn>*pool=NULL;
    try{
        pool =new threadpool<http_conn>;
    }catch(...){
        exit(-1);
    }

    // 创建客户端数组 文件描述符
    http_conn* users = new http_conn[MAX_FD];

    int listenfd =socket(AF_INET,SOCK_STREAM,0);

    //端口复用
    int reuse =1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    //绑定
    sockaddr_in saddress;   
    saddress.sin_addr.s_addr=INADDR_ANY;
    saddress.sin_port=htons(port);
    saddress.sin_family=AF_INET;
    bind(listenfd,(sockaddr*)&saddress,sizeof(saddress));

    //监听
    listen(listenfd,1024);

    //创建epoll对象 事件数组添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd=epoll_create(5);

    addfd(epollfd,listenfd,false);
    http_conn::m_epollfd=epollfd;

    while(true){
        int num=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);

        //循环遍历事件数组
        for(int i=0;i<num;i++){
            int sockfd=events[i].data.fd;
            if(sockfd==listenfd){
                struct sockaddr_in client_address;
                socklen_t len =sizeof(client_address);
                int connfd=accept(sockfd,(sockaddr*)&client_address,&len);
                if(http_conn::m_user_count>=MAX_FD){
                    //连接数满
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd,client_address);

            }else if( events[i].events&(EPOLLHUP|EPOLLRDHUP|EPOLLERR) ){
                users[sockfd].close_conn();
            }else if(events[i].events&EPOLLIN){
                if(users[sockfd].read()){
                    pool->append(users+sockfd);
                }else{
                    users[i].close_conn();
                }
            }else if(events[i].events&EPOLLOUT){
                if(!users[i].write()){
                    users[sockfd].close_conn();
                }
            }
        }

    }
    close( epollfd );
    close( listenfd );
    delete [] users;
    delete pool;
    return 0;   
    
}