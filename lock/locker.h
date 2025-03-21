#ifndef LOCKER_H
#define LOCKER_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdexcept>
#include <memory>

class sem {
public:
    sem() : impl_(std::make_unique<Impl>(0)) {}

    explicit sem(int num) : impl_(std::make_unique<Impl>(num)) {
        if (num < 0) {
            throw std::invalid_argument("Semaphore count cannot be negative");
        }
    }

    // 移动构造函数
    sem(sem&& other) noexcept = default;

    // 移动赋值操作符
    sem& operator=(sem&& other) noexcept = default;

    // 删除拷贝构造函数和赋值操作符
    sem(const sem&) = delete;
    sem& operator=(const sem&) = delete;

    bool wait() {
        std::unique_lock<std::mutex> lock(impl_->mutex_);
        impl_->cv_.wait(lock, [this]() { return impl_->count_ > 0; });
        --impl_->count_;
        return true;
    }

    bool post() {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        ++impl_->count_;
        impl_->cv_.notify_one();
        return true;
    }

private:
    struct Impl {
        Impl(int count) : count_(count) {}
        std::mutex mutex_;
        std::condition_variable cv_;
        unsigned int count_;
    };
    std::unique_ptr<Impl> impl_;  // 使用智能指针支持移动
};

class cond {
public:
    cond() = default;

    cond(const cond&) = delete;
    cond& operator=(const cond&) = delete;

    bool wait(std::mutex* mutex) {
        std::unique_lock<std::mutex> lock(*mutex, std::adopt_lock);
        cond_.wait(lock);
        lock.release();
        return true;
    }

    bool timewait(std::mutex* mutex, struct timespec t) {
        std::unique_lock<std::mutex> lock(*mutex, std::adopt_lock);
        auto duration = std::chrono::seconds(t.tv_sec) + 
                       std::chrono::nanoseconds(t.tv_nsec);
        bool result = cond_.wait_for(lock, duration) == std::cv_status::no_timeout;
        lock.release();
        return result;
    }

    bool signal() {
        cond_.notify_one();
        return true;
    }

    bool broadcast() {
        cond_.notify_all();
        return true;
    }

private:
    std::condition_variable cond_;
};

#endif