#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}
//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    // 如果设置了max_queue_size,则设置为异步
    // 如果 max_queue_size 大于等于 1，则启用异步日志，并创建一个阻塞队列 m_log_queue 来存储日志信息，
    // 同时启动一个新线程 flush_log_thread 用于异步写入日志。
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    
    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    // 获取当前日期
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 获取日志文件的完整路径
    const char *p = strrchr(file_name, '/');    // 使用 strrchr 函数查找 file_name 中最后一个 '/' 的位置
    char log_full_name[256] = {0};

    // 如果没有找到 '/'，即没有目录路径，只有日志文件名。直接拼接日期+文件名，如2024_12_13_logname
    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else    // 有目录路径，先分别获取目录路径和文件名，再拼接日期+文件名
    {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    
    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);           // 获取当前时间，返回秒级和微秒级的时间。
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);  // 将秒数转换为本地时间的 tm 结构，包含年、月、日、小时、分钟、秒等信息。
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_count++;

        // 如果当前日期与记录的日志日期不同（跨天），或者日志行数达到文件分割阈值m_split_lines。
        if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
        {
            
            char new_log[256] = {0};
            fflush(m_fp);
            fclose(m_fp);
            char tail[16] = {0};
        
            snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        
            if (m_today != my_tm.tm_mday)
            {
                snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
                m_today = my_tm.tm_mday;
                m_count = 0;
            }
            else
            {
                snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
            }
            m_fp = fopen(new_log, "a");
        }
    }

    // va_list 指向可变参数列表，用于存储用户传入的日志内容。用法见：https://www.cnblogs.com/pengdonglin137/p/3345911.html
    va_list valst;
    va_start(valst, format);

    string log_str;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 使用 snprintf 将时间戳和日志等级（s）写入 m_buf，包括年、月、日、时、分、秒、微秒。
        // 例如：2024-12-13 14:35:12.123456 [info]: 
        int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                        my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                        my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
        
        // 使用 vsnprintf 将用户传入的日志内容（format 和后续参数）追加到 m_buf 中。
        int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
        m_buf[n + m] = '\n';
        m_buf[n + m + 1] = '\0';
        log_str = m_buf;    // 将日志缓冲区 m_buf 转换为 C++ 的 std::string，存入 log_str
    }

    // 如果是异步模式（m_is_async 为 true），且日志队列未满：将格式化好的日志字符串 log_str 推送到阻塞队列中。
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        fputs(log_str.c_str(), m_fp);   // 使用 fputs 将日志写入到当前打开的日志文件中
    }

    va_end(valst);
}

void Log::flush(void)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    //强制刷新写入流缓冲区
    fflush(m_fp);
}
