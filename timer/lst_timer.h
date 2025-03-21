#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

// 连接资源
struct client_data
{
    sockaddr_in address;    // 客户端socket地址信息
    int sockfd;             // 客户端socket文件描述符
    util_timer *timer;      // 指向关联的定时器
};

class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;      // 定时器过期时间，单位为秒
    
    void (* cb_func)(client_data *);    // 定时器回调函数指针
    client_data *user_data;             // 客户端数据，用于回调时传递
    util_timer *prev;                   // 前驱定时器
    util_timer *next;                   // 后继定时器
};

// 定时器管理类，使用升序链表（以 expire 排序）来组织定时器
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);      // 添加定时器
    void adjust_timer(util_timer *timer);   // 调整定时器位置
    void del_timer(util_timer *timer);      // 删除定时器
    void tick();                            // 定时任务处理函数

private:
    void add_timer(util_timer *timer, util_timer *lst_head);    // 内部添加定时器逻辑

    util_timer *head;   // 链表头
    util_timer *tail;   // 链表尾
};

// 工具类，封装了与文件描述符操作、信号处理和定时器管理相关的功能
class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    // 对文件描述符设置非阻塞
    int setnonblocking(int fd);

    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    // 信号处理函数
    static void sig_handler(int sig);

    // 设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;           // 管道，用于信号通知
    sort_timer_lst m_timer_lst;     // 定时器链表
    static int u_epollfd;           // 全局epoll实例的fd
    int m_TIMESLOT;                 // 定时触发时间间隔（时间片）
};

// 用于定时器到期后的回调处理逻辑
void cb_func(client_data *user_data);

#endif
