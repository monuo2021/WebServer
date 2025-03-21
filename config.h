#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"

class Config
{
public:
    // 默认值常量
    static constexpr int DEFAULT_PORT = 9006;           // 默认端口号: 9006
    static constexpr int DEFAULT_LOG_WRITE = 0;         // 日志写入方式，默认同步
    static constexpr int DEFAULT_TRIG_MODE = 0;         // 触发组合模式，默认listenfd LT + connfd LT
    static constexpr int DEFAULT_LISTEN_TRIG_MODE = 0;  // listenfd触发模式，默认LT
    static constexpr int DEFAULT_CONN_TRIG_MODE = 0;    // connfd触发模式，默认LT
    static constexpr int DEFAULT_OPT_LINGER = 0;        // 优雅关闭链接，默认不使用
    static constexpr int DEFAULT_SQL_NUM = 8;           // 数据库连接池数量，默认8
    static constexpr int DEFAULT_THREAD_NUM = 8;        // 线程池内的线程数量，默认8
    static constexpr int DEFAULT_CLOSE_LOG = 0;         // 关闭日志，默认不关闭
    static constexpr int DEFAULT_ACTOR_MODEL = 0;       // 并发模型，默认是proactor

    Config()
        : PORT(DEFAULT_PORT),
          LOGWrite(DEFAULT_LOG_WRITE),
          TRIGMode(DEFAULT_TRIG_MODE),
          LISTENTrigmode(DEFAULT_LISTEN_TRIG_MODE),
          CONNTrigmode(DEFAULT_CONN_TRIG_MODE),
          OPT_LINGER(DEFAULT_OPT_LINGER),
          sql_num(DEFAULT_SQL_NUM),
          thread_num(DEFAULT_THREAD_NUM),
          close_log(DEFAULT_CLOSE_LOG),
          actor_model(DEFAULT_ACTOR_MODEL) {}
    ~Config(){};

    void parse_arg(int argc, char*argv[]);

    int getPort() { return PORT;}
    int getLOGWrite() { return LOGWrite;}
    int getTRIGMode() { return TRIGMode;}
    int getLISTENTrigmode() { return LISTENTrigmode;}
    int getCONNTrigmode() { return CONNTrigmode;}
    int getOPTLINGER() { return OPT_LINGER;}
    int getSqlNum() { return sql_num;}
    int getThreadNum() { return thread_num;}
    int getCloseLog() { return close_log;}
    int getActorModel() { return actor_model;}

private:
    int PORT;               // 端口号
    int LOGWrite;           // 日志写入方式
    int TRIGMode;           // 触发组合模式
    int LISTENTrigmode;     // listenfd触发模式
    int CONNTrigmode;       // connfd触发模式
    int OPT_LINGER;         // 优雅关闭链接
    int sql_num;            // 数据库连接池数量
    int thread_num;         // 线程池内的线程数量
    int close_log;          // 关闭日志
    int actor_model;        // 并发模型
};

#endif