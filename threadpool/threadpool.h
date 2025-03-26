#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <deque>
#include <vector>
#include <thread>
#include <condition_variable>
#include <exception>
#include "../sqlConnectionPool/sqlConnectionPool.h"

template <typename T>
class threadpool {
public:
    threadpool(int actor_model, connection_pool* connPool, int thread_number = 16, int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);
    bool append_p(T* request);
    void stop(); // 停止线程池

private:
    void run();

private:
    int m_thread_number;                    // 线程池中的线程数
    int m_max_requests;                     // 请求队列中允许的最大请求数
    std::vector<std::thread> m_threads;     // 线程池
    std::deque<T*> m_workqueue;              // 请求队列
    std::mutex m_queuelocker;               // 保护请求队列的互斥锁
    std::condition_variable m_queuecond;    // 是否有任务需要处理
    connection_pool* m_connPool;            // 数据库连接池
    int m_actor_model;                      // 模型切换
    bool m_stop;                            // 停止标志
};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number, int max_requests)
    : m_actor_model(actor_model),
      m_thread_number(thread_number),
      m_max_requests(max_requests),
      m_connPool(connPool),
      m_stop(false) {
    if (thread_number <= 0 || max_requests <= 0) {
        throw std::invalid_argument("Thread number and max requests must be positive");
    }

    m_threads.reserve(thread_number);
    for (int i = 0; i < thread_number; ++i) {
        m_threads.emplace_back(&threadpool::run, this);
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    stop();
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

template <typename T>
void threadpool<T>::stop() {
    {
        std::lock_guard<std::mutex> lock(m_queuelocker);
        m_stop = true;
    }
    m_queuecond.notify_all(); // 唤醒所有线程
}

template <typename T>
bool threadpool<T>::append(T* request, int state) {
    std::lock_guard<std::mutex> lock(m_queuelocker);
    if (m_workqueue.size() >= static_cast<size_t>(m_max_requests)) {
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuecond.notify_one();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T* request) {
    std::lock_guard<std::mutex> lock(m_queuelocker);
    if (m_workqueue.size() >= static_cast<size_t>(m_max_requests)) {
        return false;
    }
    m_workqueue.push_back(request);
    m_queuecond.notify_one();
    return true;
}

template <typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        std::unique_lock<std::mutex> lock(m_queuelocker);
        m_queuecond.wait(lock, [this] { return m_stop || !m_workqueue.empty(); });
        if (m_stop) {
            break;
        }
        if (m_workqueue.empty()) {
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        lock.unlock(); // 尽早释放锁

        if (!request) {
            continue;
        }
        if (m_actor_model == 1) {
            if (request->m_state == 0) {
                if (request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}

#endif