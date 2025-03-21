/*
 * 同步日志
 * 在默认情况下，日志是同步的。每次调用 write_log 时，日志会直接写入文件，并调用 flush() 强制刷新缓冲区。
 * 这样做的优点是简单，缺点是会增加 I/O 操作的阻塞，
 * 从而降低程序性能。
 */

/*
 * 异步日志
 * 如果配置了异步日志（通过 max_queue_size 参数），日志会先被写入阻塞队列，
 * 然后由一个单独的线程从队列中取出日志并写入文件。
 * 这样做的优点是大大减少了写日志的 I/O 阻塞，提高了程序性能。
 */

/*
 * 阻塞队列
 * 日志首先写入阻塞队列，由后台线程异步处理。这样可以避免在写日志时阻塞主线程，提升程序的并发性能。
 */

/*
 * 后台线程
 * 后台线程通过 flush_log_thread 函数不断从阻塞队列中读取日志，并将日志写入文件。
 * 这个线程的作用是异步处理日志写入，避免阻塞主线程。
 */

/*
 * 日志分割
 * 日志系统支持日志文件按天分割，m_today 记录当前日期。每当日志行数达到 m_split_lines 时，
 * 会创建一个新的日志文件来继续写入。这有助于防止单个日志文件过大，同时也方便日志的管理和查看。
 */

#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    // 静态成员函数，用于异步日志的写入
    static void *flush_log_thread(void *args)
    {
        Log::get_instance()->async_write_log();
    }
    // 初始化日志系统的配置，包括日志文件名、是否关闭日志、日志缓冲区大小、日志分割行数和最大队列大小。
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log()
    {
        string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->pop(single_log))
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            fputs(single_log.c_str(), m_fp);
        }
    }

private:
    char dir_name[128]; // 存储日志文件所在目录的路径
    char log_name[128]; // 日志文件名
    int m_split_lines;  // 单个日志文件最大行数。如果日志文件超过该行数，则进行日志文件切割。
    int m_log_buf_size; // 日志缓冲区大小，单位为字节。日志会首先写入缓冲区，达到一定大小后再写入文件。
    long long m_count;  // 日志行数，记录当前日志写入的行数。
    int m_today;        // 记录当前是哪一天，用于日志文件的按天分割。
    FILE *m_fp;         // 文件指针，指向日志文件，用于写入日志。
    char *m_buf;        // 日志缓冲区，临时存储日志内容。
    block_queue<string> *m_log_queue; // 阻塞队列，用于异步写日志。将日志内容放入队列，后台线程会从队列中读取并写入文件。
    bool m_is_async;                  // 是否启用异步日志标志位。如果为 true，则日志写入是异步的。
    std::mutex m_mutex;     // 锁对象，用于保护日志文件的写入，防止多线程下的竞争条件。
    int m_close_log; // 关闭日志的标志。如果为 1，表示关闭日志功能，任何日志写入操作都会被忽略。
};

#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

#endif
