#include "log.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <cstdarg>
#include <filesystem>

Log::Log() : _count(0), _is_async(false) {}

Log::~Log() {
    if (_fp.is_open()) {
        _fp.close();
    }
}

bool Log::init(const std::string& file_name, int split_lines, int max_queue_size) {
    // 若队列大小≥1，启用异步模式
    if (max_queue_size >= 1) {
        _is_async = true;
        _log_queue = std::make_unique<block_queue<std::string>>(max_queue_size);
        std::thread flush_thread(flush_log_thread, nullptr);
        flush_thread.detach(); // 在后台运行
    }

    // 配置分割行数。
    _split_lines = split_lines;

    auto now = std::chrono::system_clock::now();                // 获取当前系统时间点
    std::time_t t = std::chrono::system_clock::to_time_t(now);  // 将时间点转换为 time_t 类型
    // std::tm my_tm = *std::localtime(&t);                     // 将 time_t 转换为本地时间的 tm 结构体，但此函数线程不安全
    std::tm my_tm;
    localtime_r(&t, &my_tm);                                    // 线程安全的替代方案

    size_t pos = file_name.find_last_of('/');   
    if (pos == std::string::npos) {     // 如果路径中没有 /，则文件名即为 file_name，目录名为当前目录 ./serverLogs/
        log_name = file_name;
        dir_name = "./serverLogs/";
    } else {    // 如果路径中有 /，则提取文件名和目录名。
        log_name = file_name.substr(pos + 1);
        dir_name = file_name.substr(0, pos + 1);
    }

    // 记录当前日期的天数（1~31）
    _today = my_tm.tm_mday;

    // 生成带时间戳的日志文件名
    std::ostringstream oss;
    oss << dir_name << std::put_time(&my_tm, "%Y_%m_%d_") << log_name;
    // 使用 C++17 的 std::filesystem::create_directories 确保目录存在。
    std::filesystem::create_directories(dir_name);
    // 以追加模式打开日志文件，并检查是否成功。
    _fp.open(oss.str(), std::ios::app);
    if (!_fp.is_open()) {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char* format, ...) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm my_tm;
    localtime_r(&t, &my_tm);                                    // 线程安全的替代方案
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    const char* level_str;
    switch (level) {
        case 0: level_str = "[debug]:"; break;
        case 1: level_str = "[info]:"; break;
        case 2: level_str = "[warn]:"; break;
        case 3: level_str = "[error]:"; break;
        default: level_str = "[info]:"; break;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _count++;

        // 如果当前日期与记录的日志日期不同（跨天），或者日志行数达到文件分割阈值_split_lines。
        if (_today != my_tm.tm_mday || _count % _split_lines == 0) {
            std::ostringstream new_log;
            new_log << dir_name << std::put_time(&my_tm, "%Y_%m_%d_") << log_name;
            if (_today != my_tm.tm_mday) {
                _today = my_tm.tm_mday;
                _count = 0;
            } else {
                new_log << "." << (_count / _split_lines);
            }
            _fp.close();
            _fp.open(new_log.str(), std::ios::app);
        }
    }

    // va_list 指向可变参数列表，用于存储用户传入的日志内容。用法见：https://www.cnblogs.com/pengdonglin137/p/3345911.html
    va_list valst;
    va_start(valst, format);

    std::ostringstream log_stream;
    log_stream << std::put_time(&my_tm, "%Y-%m-%d %H:%M:%S") << "."
               << std::setfill('0') << std::setw(6) << us.count() << " "
               << level_str << " ";

    char temp[1024];
    vsnprintf(temp, sizeof(temp), format, valst);
    log_stream << temp << '\n';

    va_end(valst);

    std::string log_str = log_stream.str();

    // 如果是异步模式（_is_async 为 true），且日志队列未满：将格式化好的日志字符串 log_str 推送到阻塞队列中。
    if (_is_async && !_log_queue->full()) {
        _log_queue->push(log_str);
    } else {
        std::lock_guard<std::mutex> lock(_mutex);
        _fp << log_str;
        _fp.flush();
    }
}

void Log::flush() {
    std::lock_guard<std::mutex> lock(_mutex);
    _fp.flush();
}

void Log::write_batch(std::vector<std::string>& logs) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& log : logs) {
        _fp << log;
    }
    _fp.flush();
    logs.clear();
}

void Log::async_write_log() {
    std::cout << "async_write_log start" << std::endl;
    std::vector<std::string> log_strs;
    log_strs.reserve(300);
    const size_t batch_size = std::min<size_t>(16, _log_queue->max_size() / 10); // 动态调整

    while (true) {
        std::string single_log;
        // 使用带超时的 pop，等待 100ms
        while (_log_queue->pop(single_log, 100)) {
            log_strs.emplace_back(std::move(single_log));
            if (log_strs.size() >= batch_size) {
                write_batch(log_strs);
            }
        }
        // 处理剩余日志
        if (!log_strs.empty()) {
            write_batch(log_strs);
        }
        // 队列为空时休眠
        if (log_strs.empty() && _log_queue->empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}