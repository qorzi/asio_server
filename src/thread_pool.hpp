#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    void enqueue_task(const std::function<void()>& task); // 작업 추가
    void add_worker();                                   // 워커 스레드 추가
    std::size_t worker_count() const;                   // 현재 워커 스레드 수 반환

private:
    void worker_thread();                                // 워커 스레드 작업 처리 함수

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;           // 작업 큐

    std::mutex mutex_;
    std::condition_variable cond_var_;
    bool stop_;                                         // 스레드풀 중지 플래그
};

#endif // THREAD_POOL_HPP
