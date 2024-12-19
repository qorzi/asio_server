#include "thread_pool.hpp"

ThreadPool::ThreadPool() : stop_(false) {
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers_.emplace_back([this]() { worker_thread(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_var_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue_task(const std::string& task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(task);
    }
    cond_var_.notify_one();
}

void ThreadPool::worker_thread() {
    while (true) {
        std::string task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        // Process task (placeholder logic)
        std::cout << "Processing task: " << task << std::endl;
    }
}