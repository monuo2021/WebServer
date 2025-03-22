#ifndef LOCKER_H
#define LOCKER_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdexcept>
#include <memory>

class sem {
public:
    // 默认构造函数，初始计数为 0
    sem() : count_(0) {}

    // 带初始计数的构造函数
    explicit sem(int num) : count_(num) {
        if (num < 0) {
            throw std::invalid_argument("Semaphore count cannot be negative");
        }
    }

    // 移动构造函数
    sem(sem&& other) noexcept 
        : mutex_{}, cv_{}, count_(other.count_) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        other.count_ = 0;  // 被移动的对象重置为初始状态
    }

    // 移动赋值操作符
    sem& operator=(sem&& other) noexcept {
        if (this != &other) {
            std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
            std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
            std::lock(lock_this, lock_other);  // 避免死锁
            count_ = other.count_;
            other.count_ = 0;
        }
        return *this;
    }

    // 删除拷贝构造函数和赋值操作符
    sem(const sem&) = delete;
    sem& operator=(const sem&) = delete;

    // 等待信号量
    bool wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return count_ > 0; });
        --count_;
        return true;
    }

    // 释放信号量
    bool post() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_one();
        return true;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    unsigned int count_;
};

#endif