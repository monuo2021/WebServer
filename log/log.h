#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <vector>
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
    bool init(const std::string& file_name, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...);

    void flush();

private:
    Log();
    ~Log();
    void write_batch(std::vector<std::string>& logs);
    void async_write_log();

private:
    std::string dir_name;           // 日志文件目录路径
    std::string log_name;           // 日志文件名
    int _split_lines;              // 日志文件最大行
    long long _count;              // 已写入的日志行数
    int _today;                    // 当前日期，用于日志分割
    std::ofstream _fp;             // 日志输出文件流
    std::unique_ptr<block_queue<std::string>> _log_queue; // 异步日志阻塞队列
    bool _is_async;                // 是否异步写入日志
    std::mutex _mutex;             // 线程安全的互斥锁
};

#define LOG_DEBUG(format, ...) if (!m_close_log) { Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_INFO(format, ...)  if (!m_close_log) { Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_WARN(format, ...)  if (!m_close_log) { Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush(); }
#define LOG_ERROR(format, ...) if (!m_close_log) { Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush(); }

#endif