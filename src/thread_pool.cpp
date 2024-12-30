#include "thread_pool.hpp"
#include <iostream>

// 생성자: 초기 스레드 수만큼 워커 생성
ThreadPool::ThreadPool(std::size_t num_threads) : stop_(false) {
    for (std::size_t i = 0; i < num_threads; ++i) {
        add_worker();
    }
}

// 소멸자: 모든 스레드 종료
ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_var_.notify_all(); // 모든 대기 중인 스레드를 깨움

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// 작업 추가
void ThreadPool::enqueue_task(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(task);
    }
    cond_var_.notify_one();
}

// 워커 스레드 추가
void ThreadPool::add_worker() {
    workers_.emplace_back([this]() { worker_thread(); });
}

// 현재 워커 스레드 수 반환
std::size_t ThreadPool::worker_count() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_)); // TODO: 타입 캐스팅 없이 작동하도록 수정해야 함.
    return workers_.size();
}

// 워커 스레드의 작업 처리
void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
                return; // 종료 조건
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // 작업 실행
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "Error while executing task: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error while executing task." << std::endl;
        }
    }
}
