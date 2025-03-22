/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class block_queue
{
public:
    explicit block_queue(int max_size = 1000) 
        : m_array(std::make_unique<T[]>(max_size)), 
          m_max_size(max_size), 
          m_size(0), 
          m_front(-1), 
          m_back(-1) {
        if (max_size <= 0) {
            throw std::invalid_argument("Queue size must be positive");
        }
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        m_size = 0;
        m_front = -1;
        m_back = -1;
        cv_.notify_all();
    }

    ~block_queue() = default;

    //判断队列是否满了
    bool isFull()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_size >= m_max_size;
    }
    //判断队列是否为空
    bool isEmpty() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_size == 0;
    }

    int size() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_size;
    }

    int max_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_max_size;
    }

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (m_size >= m_max_size) {
            return false;
        }
        
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;  // 假设 T 支持拷贝赋值
        ++m_size;
        cv_.notify_all();  // 通知消费者有新数据
        return true;
    }

    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]() { return m_size > 0; });
        m_front = (m_front + 1) % m_max_size;
        item = std::move(m_array[m_front]);  // 使用移动语义提高效率
        --m_size;
        return true;
    }

    // 增加了超时处理
    bool pop(T& item, int ms_timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (m_size <= 0) {
            if (cv_.wait_for(lock, std::chrono::milliseconds(ms_timeout), 
                             [this]() { return m_size > 0; }) == false) {
                return false;  // 超时返回
            }
        }
        m_front = (m_front + 1) % m_max_size;
        item = std::move(m_array[m_front]);
        --m_size;
        return true;
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;

    std::unique_ptr<T[]> m_array;  // 智能指针管理数组
    int m_size;
    const int m_max_size;
    int m_front;
    int m_back;
};

#endif
