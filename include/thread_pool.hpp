#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <iostream>

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();
    void enqueue_task(const std::string& task);

private:
    void worker_thread();

    std::vector<std::thread> workers_;
    std::queue<std::string> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    bool stop_;
};

#endif // THREAD_POOL_HPP