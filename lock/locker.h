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

class cond {
public:
    cond() = default;

    cond(const cond&) = delete;
    cond& operator=(const cond&) = delete;

    void wait(std::mutex* mutex) {
        std::unique_lock<std::mutex> lock(*mutex, std::adopt_lock);
        cond_.wait(lock);
        lock.release();
    }

    // 在指定的时间内等待条件变量被通知，如果在超时前被通知则返回 true，否则返回 false。​
    bool timewait(std::mutex* mutex, int ms_timeout) {
        // 使用 std::unique_lock 管理互斥锁，并采用 std::defer_lock 标记
        // 表示在构造时不立即锁定互斥锁，而是在后续手动锁定
        std::unique_lock<std::mutex> lock(*mutex, std::defer_lock);
    
        // 将超时时间（毫秒）转换为 std::chrono::duration
        auto timeout_duration = std::chrono::milliseconds(ms_timeout);
    
        // 等待条件变量，带超时
        // 使用 wait_for 而不是 wait_until，直接传入超时时间
        bool result = cond_.wait_for(lock, timeout_duration) == std::cv_status::no_timeout;
    
        // 不需要手动释放锁，std::unique_lock 会在析构时自动解锁
        return result;
    }

    void signal() {
        cond_.notify_one();
    }

    void broadcast() {
        cond_.notify_all();
    }

private:
    std::condition_variable cond_;
};

#endif