#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <chrono>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    void enqueue(const std::function<void()>& task);
    void wait_for_completion();
    ~ThreadPool();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> active_tasks;
};
