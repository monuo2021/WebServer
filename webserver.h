#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , std::string user, std::string passWord, std::string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int m_port;
    char *m_root;
    int m_log_write;        // 日志写入方式，=0 默认同步
    int m_close_log;        // 是否关闭日志，=0 默认不关闭
    int m_actormodel;

    int m_pipefd[2];
    int m_epollfd;          // 用于epoll事件通知的文件描述符
    http_conn *users;       // 存储客户端连接的http_conn对象，每个连接一个http_conn对象来处理HTTP请求。

    //数据库相关
    connection_pool *m_connPool;    // 管理数据库连接池
    std::string m_user;         //登陆数据库用户名
    std::string m_passWord;     //登陆数据库密码
    std::string m_databaseName; //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;       // m_OPT_LINGER = 0：快速关闭模式：服务器关闭连接时不会等待未发送的数据完成传输，直接丢弃未处理的数据。m_OPT_LINGER = 1：优雅关闭模式：服务器会在关闭连接前等待未发送数据的发送完成。通过设置 linger 选项，确保数据能够尽可能地发送到客户端。
    int m_TRIGMode;         // 触发模式，决定了epoll的工作方式（ET模式或LT模式）
    int m_LISTENTrigmode;   // 监听套接字的触发模式
    int m_CONNTrigmode;     // 连接套接字的触发模式

    //定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif
