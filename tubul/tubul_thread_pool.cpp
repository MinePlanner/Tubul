
#include "tubul_thread_pool.h"


namespace TU
{


    ThreadPool::ThreadPool(size_t thread_count) :
            running_(false),
            tasks_total_(0),
            thread_count_((thread_count > 0) ? thread_count : std::thread::hardware_concurrency()),
            threads_(std::make_unique<std::thread[]>(thread_count_)),
            waiting_(false)
    {
        for (size_t i = 0; i < thread_count_; ++i)
        {
            threads_[i] = std::thread(&ThreadPool::workerFn, this);
        }
        running_ = true;
    }

    ThreadPool::~ThreadPool() {
        waitForTasks();
        running_ = false;
        task_available_cv_.notify_all();
        for (size_t i = 0; i < thread_count_; ++i)
        {
            threads_[i].join();
        }
    }

    void ThreadPool::waitForTasks() {
        waiting_ = true;
        std::unique_lock<std::mutex> tasks_lock(tasks_mutex_);
        task_done_cv_.wait(tasks_lock, [this] { return (tasks_total_ == 0); });
        waiting_ = false;
    }

    size_t ThreadPool::threadCount() const {
        return thread_count_;
    }

}