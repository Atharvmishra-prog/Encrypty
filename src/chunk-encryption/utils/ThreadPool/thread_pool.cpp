#include "./thread_pool.h"

ThreadPool::ThreadPool(size_t num_threads) : stop(false), active_tasks(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this]() {
            // while true loop to keep the thread alive since there are active tasks to work on
            while (true) {
                std::function<void()> task;

                {
                    // context lock to protect the queue
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this]() {
                        return this->stop || !this->tasks.empty();
                    });

                    if (this->stop && this->tasks.empty())
                        return;

                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                    ++active_tasks;
                }

                task();
                --active_tasks;
            }
        });
    }
}

void ThreadPool::enqueue(const std::function<void()>& task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(task);
    }
    condition.notify_one(); // Notify one waiting thread in the pool
}

void ThreadPool::wait_for_completion() {
    while (true) {
        if (tasks.empty() && active_tasks.load() == 0)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}