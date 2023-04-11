
#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>

namespace TU
{


    class ThreadPool
    {
    public:

        explicit ThreadPool(size_t thread_count = 0);

        ~ThreadPool();

        [[maybe_unused]] [[nodiscard]]
        size_t threadCount() const;

        /**
         * @brief Push a function with zero or more arguments, but no return value, into the task queue.
         * Does not return a future, so the user must use waitForTasks() or some other method to ensure
         * that the task finishes executing, otherwise bad things will happen.
         *
         * @tparam F The type of the function.
         * @tparam A The types of the arguments.
         * @param task The function to push.
         * @param args The zero or more arguments to pass to the function. Note that if the task is a
         * class member function, the first argument must be a pointer to the object, i.e. &object
         * (or this), followed by the actual arguments.
         */
        template <typename F, typename... A>
        void push_task(F&& task, A&&... args)
        {
            std::function<void()> task_function = std::bind(std::forward<F>(task), std::forward<A>(args)...);
            {
                const std::scoped_lock tasks_lock(tasks_mutex_);
                tasks_.push(task_function);
            }
            ++tasks_total_;
            task_available_cv_.notify_one();
        }

        /**
         * @brief Submit a function with zero or more arguments into the task queue. If the function
         * has a return value, get a future for the eventual returned value. If the function has no
         * return value, get an std::future<void> which can be used to wait until the task finishes.
         *
         * @tparam F The type of the function.
         * @tparam A The types of the zero or more arguments to pass to the function.
         * @tparam R The return type of the function (can be void).
         * @param task The function to submit.
         * @param args The zero or more arguments to pass to the function. Note that if the task is a
         * class member function, the first argument must be a pointer to the object, i.e. &object (or this),
         * followed by the actual arguments.
         * @return A future to be used later to wait for the function to finish executing and/or obtain
         * its returned value if it has one.
         */
        template <typename F, typename... A, typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>
        [[nodiscard]] std::future<R> submit(F&& task, A&&... args)
        {
            std::function<R()> task_function = std::bind(std::forward<F>(task), std::forward<A>(args)...);
            std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
            push_task(
                    [task_function, task_promise]
                    {
                        try
                        {
                            if constexpr (std::is_void_v<R>)
                            {
                                std::invoke(task_function);
                                task_promise->set_value();
                            }
                            else
                            {
                                task_promise->set_value(std::invoke(task_function));
                            }
                        }
                        catch (...)
                        {
                            task_promise->set_exception(std::current_exception());
                        }
                    });
            return task_promise->get_future();
        }

        /**
         * @brief Wait for tasks_ to be completed. Normally, this function waits for all tasks, both
         * those that are currently running in the threads_ and those that are still waiting in the queue.
         * Note: To wait for just one specific task, use submit() instead, and call the wait() member
         * function of the generated future.
         */
        void waitForTasks();

    private:

        /**
         * @brief A worker function to be assigned to each thread in the pool. Waits until it is notified by
         * push_task() that a task is available, and then retrieves the task from the queue and executes it. Once
         * the task finishes, the worker notifies waitForTasks() in case it is waiting_.
         */
        void workerFn()
        {
            while (running_)
            {
                std::function<void()> task;
                std::unique_lock<std::mutex> tasks_lock(tasks_mutex_);
                task_available_cv_.wait(tasks_lock, [this] { return !tasks_.empty() || !running_; });
                if (running_)
                {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    tasks_lock.unlock();
                    task();
                    tasks_lock.lock();
                    --tasks_total_;
                    if (waiting_)
                        task_done_cv_.notify_one();
                }
            }
        }



        /**
         * @brief An atomic variable indicating to the workers to keep running_. When set to false, the
         * workers permanently stop working.
         */
        std::atomic<bool> running_;

        /**
         * @brief A condition variable used to notify worker() that a new task has become available.
         */
        std::condition_variable task_available_cv_;

        /**
         * @brief A condition variable used to notify waitForTasks() that a tasks_ is done.
         */
        std::condition_variable task_done_cv_;

        /**
         * @brief A queue of tasks_ to be executed by the threads_.
         */
        std::queue<std::function<void()>> tasks_ = {};

        /**
         * @brief An atomic variable to keep track of the total number of unfinished
         * tasks either still in the queue, or running in a thread.
         */
        std::atomic<size_t> tasks_total_;

        /**
         * @brief A mutex to synchronize access to the task queue by different threads_.
         */
        mutable std::mutex tasks_mutex_;

        /**
         * @brief The number of threads_ in the pool.
         */
        size_t thread_count_;

        /**
         * @brief A smart pointer to manage the memory allocated for the threads_.
         */
        std::unique_ptr<std::thread[]> threads_;

        /**
         * @brief An atomic variable indicating that waitForTasks() is active and expects to be
         * notified whenever a task is done.
         */
        std::atomic<bool> waiting_;
    };


}
