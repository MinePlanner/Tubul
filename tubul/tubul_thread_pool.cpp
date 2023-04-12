
#include "tubul_thread_pool.h"


namespace TU
{


    ThreadPool::ThreadPool(size_t thread_count) :
            tasks_total_(0),
            thread_count_((thread_count > 0) ? thread_count : std::thread::hardware_concurrency()),
            threads_(std::make_unique<std::thread[]>(thread_count_))
    {
        running_.test_and_set();
        for (size_t i = 0; i < thread_count_; ++i)
        {
            threads_[i] = std::thread(&ThreadPool::workerFn, this);
        }
    }

    ThreadPool::~ThreadPool() {
        waitForTasks();
        running_.clear();
        task_available_cv_.notify_all();
        for (size_t i = 0; i < thread_count_; ++i)
        {
            threads_[i].join();
        }
    }

    void ThreadPool::waitForTasks() {
        waiting_.test_and_set();
        std::unique_lock<std::mutex> tasks_lock(tasks_mutex_);
        task_done_cv_.wait(tasks_lock, [this] { return (tasks_total_ == 0); });
        waiting_.clear();
    }

    size_t ThreadPool::threadCount() const {
        return thread_count_;
    }

    void ThreadPool::workerFn() {
        while (running_.test())
        {
            std::function<void()> task;
            std::unique_lock<std::mutex> tasks_lock(tasks_mutex_);
            task_available_cv_.wait(tasks_lock, [this] { return !tasks_.empty() || !running_.test(); });
            if (running_.test())
            {
                task = std::move(tasks_.front());
                tasks_.pop();
                tasks_lock.unlock();
                task();
                tasks_lock.lock();
                --tasks_total_;
                if (waiting_.test())
                    task_done_cv_.notify_one();
            }
        }
    }

}