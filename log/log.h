#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include "block_queue.h"

class Log {
public:
    static Log* get_instance() {
        static Log instance; // 单例模式，懒初始化
        return &instance;
    }

    static void flush_log_thread(void*) {
        Log::get_instance()->async_write_log();
    }

    // 修改 file_name 参数为 std::string 类型
    bool init(const std::string& file_name, int close_log, int log_buf_size = 8192, 
              int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...);

    void flush();

private:
    Log();
    ~Log();
    void async_write_log();

private:
    std::string dir_name;           // 日志文件目录路径
    std::string log_name;           // 日志文件名
    int m_split_lines;              // 日志文件最大行数
    int m_log_buf_size;             // 日志缓冲区大小
    long long m_count;              // 已写入的日志行数
    int m_today;                    // 当前日期，用于日志分割
    std::ofstream m_fp;             // 日志输出文件流
    std::string m_buf;              // 日志格式化缓冲区
    block_queue<std::string>* m_log_queue; // 异步日志阻塞队列
    bool m_is_async;                // 是否异步写入日志
    std::mutex m_mutex;             // 线程安全的互斥锁
    int m_close_log;                // 是否关闭日志（0 = 启用）
};

#define LOG_DEBUG(format, ...) if (!m_close_log) { Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_INFO(format, ...)  if (!m_close_log) { Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_WARN(format, ...)  if (!m_close_log) { Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_ERROR(format, ...) if (!m_close_log) { Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush(); }

#endif