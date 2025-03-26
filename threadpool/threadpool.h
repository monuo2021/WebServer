#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <list>
#include <vector>
#include <memory>
#include <thread>
#include <exception>
#include "../lock/locker.h"
#include "../sqlConnectionPool/sqlConnectionPool.h"

template <typename T>
class threadpool {
public:
    threadpool(int actor_model, connection_pool* connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);
    bool append_p(T* request);

private:
    void run();

private:
    int m_thread_number;                    // 线程池中的线程数
    int m_max_requests;                     // 请求队列中允许的最大请求数
    std::vector<std::thread> m_threads;     // 线程池
    std::list<T*> m_workqueue;              // 请求队列
    std::mutex m_queuelocker;               // 保护请求队列的互斥锁
    sem m_queuestat;                        // 是否有任务需要处理
    connection_pool* m_connPool;            // 数据库连接池
    int m_actor_model;                      // 模型切换
};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number, int max_requests)
    : m_actor_model(actor_model),
      m_thread_number(thread_number),
      m_max_requests(max_requests),
      m_connPool(connPool) {
    if (thread_number <= 0 || max_requests <= 0) {
        throw std::invalid_argument("Thread number and max requests must be positive");
    }

    m_threads.reserve(thread_number);
    for (int i = 0; i < thread_number; ++i) {
        m_threads.emplace_back(&threadpool::run, this);
        m_threads.back().detach();
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    // detach 的线程无需手动清理
}

template <typename T>
bool threadpool<T>::append(T* request, int state) {
    std::lock_guard<std::mutex> lock(m_queuelocker);
    if (m_workqueue.size() >= static_cast<size_t>(m_max_requests)) {
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuestat.post();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T* request) {
    std::lock_guard<std::mutex> lock(m_queuelocker);
    if (m_workqueue.size() >= static_cast<size_t>(m_max_requests)) {
        return false;
    }
    m_workqueue.push_back(request);
    m_queuestat.post();
    return true;
}

template <typename T>
void threadpool<T>::run() {
    while (true) {
        m_queuestat.wait();
        std::lock_guard<std::mutex> lock(m_queuelocker);
        if (m_workqueue.empty()) {
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
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