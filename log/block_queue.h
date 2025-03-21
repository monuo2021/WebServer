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
    block_queue(int max_size = 1000)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    ~block_queue()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (m_array != NULL)
            delete [] m_array;
    }
    //判断队列是否满了
    bool full() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_size >= m_max_size;
    }
    //判断队列是否为空
    bool empty() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return m_size == 0;
    }
    //返回队首元素
    bool front(T &value) 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (m_size == 0)
        {
            return false;
        }
        value = m_array[m_front];
        return true;
    }
    //返回队尾元素
    bool back(T &value) 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (m_size == 0)
        {
            return false;
        }
        value = m_array[m_back];
        return true;
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
    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (m_size >= m_max_size)
        {
            m_cond.broadcast();
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        m_size++;

        m_cond.broadcast();
        return true;
    }
    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        while (m_size <= 0)
        {
            if (!m_cond.wait(&mtx_))
            {
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        return true;
    }

    //增加了超时处理
    bool pop(T &item, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);

        std::lock_guard<std::mutex> lock(mtx_);
        if (m_size <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timewait(&mtx_, t))
            {
                return false;
            }
        }

        if (m_size <= 0)
        {
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        return true;
    }

private:
    std::mutex mtx_;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;
};

#endif
