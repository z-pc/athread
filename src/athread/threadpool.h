//////////////////////////////////////////////////////////////////////////
// Copyright 2022 Le Xuan Tuan Anh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////

#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

#include "noncopyable.h"
#include "runnable.h"
#include "worker.h"

namespace at
{
using TaskQueue = std::queue<IRunnable*>;

/**
 * @brief A thread pool that manages a pool of worker threads for executing tasks concurrently.
 *
 * The `ThreadPool` class is responsible for managing a set of worker threads that execute tasks concurrently.
 * It creates, manages, and terminates threads in a controlled manner to avoid creating excessive threads while
 * ensuring that tasks are executed efficiently.
 *
 * The pool will create a set of core threads upon initialization. If additional threads are required,
 * up to a maximum specified number of threads can be created. Threads will wait for new tasks in a queue
 * and execute them as they become available. Once a task is completed, the worker thread will either
 * continue to the next task or be terminated based on the pool's configuration.
 *
 * If the number of threads reaches the core size, additional threads will be considered "seasonal"
 * and will be destroyed if idle for too long.
 *
 * @note The thread pool does not take ownership of the tasks. The tasks must manage their own memory.
 */
class ThreadPool : public at::noncopyable_::noncopyable
{
    friend class IWorker;
    friend class ThreadPoolWorker;
    friend class ThreadSeasonalWorker;

public:
    /**
     * @brief Constructs a ThreadPool instance.
     *
     * The constructor initializes the thread pool with a specified number of core threads and a maximum
     * number of threads. Optionally, an alive time can be provided for seasonal workers, and a flag can
     * indicate whether the pool should wait for a start signal.
     *
     * @param core_thread_count The number of core threads that should always remain alive. Default is 2.
     * @param max_thread_count The maximum number of threads in the pool. If set to 0, it will not limit the number of
     * threads.
     * @param alive_seasonal_time The duration for which seasonal workers stay alive if idle. Default is 60 seconds.
     * @param wait_for_start_signal A flag indicating whether the pool should wait for a start signal before executing
     * tasks. Default is false.
     */
    ThreadPool(std::uint32_t core_thread_count = 2, std::uint32_t max_thread_count = 0,
               const std::chrono::nanoseconds& alive_seasonal_time = std::chrono::seconds(60),
               bool wait_for_start_signal = false);

    /**
     * @brief Destructor for the ThreadPool.
     *
     * The destructor ensures that all worker threads are cleaned up and that the pool is properly shut down.
     */
    virtual ~ThreadPool();

    /**
     * @brief Pushes a runnable task to the thread pool for execution.
     *
     * The thread pool takes ownership of the memory of the runnable and ensures it is executed, so the caller must not
     * manually delete it.
     *
     * @param runnable A pointer to an `IRunnable` task that needs to be executed.
     * @return true if the task was successfully added, false otherwise.
     */
    virtual bool push(IRunnable* runnable);

    /**
     * @brief Pushes a callable object (e.g., function, lambda) to the thread pool.
     *
     * This version of the `push` method allows passing a callable object, which will be wrapped in a
     * runnable and executed by the thread pool.
     *
     * The thread pool takes ownership of the memory of the runnable and ensures it is executed, so the caller must not
     * manually delete it.
     *
     * @tparam Fn The type of the callable (function, lambda, etc.).
     * @tparam Args The types of the arguments passed to the callable.
     * @param fn The callable object to be executed.
     * @param args The arguments to be forwarded to the callable.
     * @return true if the task was successfully added, false otherwise.
     */
    template <class Fn, class... Args, std::enable_if_t<std::is_invocable_v<Fn, Args...>, bool> = true>
    bool push(Fn&& fn, Args&&... args);

    /**
     * @brief Adds a runnable to the thread pool by constructing it in place using forwarded arguments.
     *
     * This method allows creating and adding a runnable to the pool using constructor arguments forwarded
     * to the runnable's constructor.
     *
     * The thread pool takes ownership of the memory of the runnable and ensures it is executed, so the caller
     * must not manually delete it.
     *
     * @tparam _Runnable The type of the runnable to be created.
     * @tparam Args The types of the arguments forwarded to the runnable's constructor.
     * @param args Arguments to forward to the constructor of the runnable.
     * @return true if the task was successfully added, false otherwise.
     */
    template <class _Runnable, class... Args>
    bool emplace(Args&&... args);

    /**
     * @brief Clears all tasks currently waiting in the queue and not yet executed.
     *
     * This function removes all `IRunnable` objects that are still in the queue and have not been assigned to workers.
     * Note that this does not affect tasks currently being executed by workers.
     *
     * @warning Be cautious when using this method, as it will discard all pending tasks permanently.
     */
    void clear();

    /**
     * @brief Starts the execution of tasks in the thread pool.
     *
     * This method signals the thread pool to start processing tasks. It will not proceed until a start signal
     * has been received if the `waitForStartSignal` flag is set to true.
     */
    virtual void start();

    /**
     * @brief Waits for all worker threads to finish executing their tasks.
     *
     * This method blocks until all worker threads have completed their tasks and have exited.
     */
    virtual void wait();

    /**
     * @brief Terminates the thread pool, stopping the execution of remaining tasks.
     *
     * This method signals the thread pool to stop executing the remaining tasks in the queue.
     * However, it does not immediately stop the `IRunnable` tasks that are currently being executed by the worker
     * threads. Those tasks will continue to run until they are completed. Once all tasks have been completed, the
     * thread pool will terminate. If the `alsoWait` flag is true, the method will block until all tasks are finished
     * and workers are exited.
     *
     * @param alsoWait A flag that indicates whether the function should also call `wait()` to block
     *                 until all tasks are completed. Default is true.
     */
    virtual void terminate(bool _also_wait = true);

    /**
     * @brief Checks whether the thread pool is able to execute a new task.
     *
     * This method checks if the pool has any available workers or if the pool is in a state where
     * it is ready to accept new tasks.
     *
     * @return true if the pool is ready to accept a new task, false otherwise.
     */
    virtual bool executable() const;

    bool empty();

protected:
    /**
     * @brief Creates a specified number of worker threads.
     *
     * This method creates the specified number of worker threads in the pool.
     *
     * @param count The number of worker threads to create.
     */
    virtual void create_worker(std::uint32_t count);

    /**
     * @brief Creates a specified number of seasonal worker threads.
     *
     * This method creates the specified number of seasonal worker threads, which will be destroyed
     * if idle for too long (as defined by `aliveTime`).
     *
     * @param count The number of seasonal worker threads to create.
     * @param aliveTime The duration for which the seasonal workers remain alive if idle.
     */
    virtual void create_seasonal_worker(std::uint32_t count, const std::chrono::nanoseconds& alive_seasonal_time);

    /**
     * @brief Cleans up completed workers that are no longer needed.
     *
     * This method removes workers who have completed their tasks and are no longer active in the pool.
     */
    void clean_complete_workers();

    void reset();
    std::uint32_t generate_worker_uid() const;

    std::uint32_t _core_thread_count;
    std::uint32_t _max_thread_count;
    std::chrono::nanoseconds _alive_seasonal_time;
    at::TaskQueue _task_queue;
    std::mutex _task_queue_mutex;
    std::condition_variable _work_available_condition;
    std::atomic_bool _termination_flag;
    std::atomic_bool _wait_for_start_signal;
    std::vector<std::unique_ptr<at::WorkerContext>> _worker_contexts;  ///< Contexts for worker threads.
};

/**
 * @class ThreadPoolFixed
 * @brief A thread pool that executes tasks only after a start signal is given.
 *
 * The `ThreadPoolFixed` class behaves similarly to `ThreadPool`, but it ensures that tasks are not executed
 * until the start signal is explicitly received. Once the start signal is given, the pool will execute tasks
 * in the queue and exit when there are no more tasks or when the termination signal is received.
 * New tasks can still be added and executed after the start signal is given, as long as the thread pool has not exited.
 */
class ThreadPoolFixed : public ThreadPool
{
public:
    /**
     * @brief Constructs a fixed thread pool with the given number of core threads.
     *
     * @param coreSize The number of core threads that should remain active in the pool.
     */
    explicit ThreadPoolFixed(std::uint32_t coreSize);

    /**
     * @brief Destructor.
     *
     * Cleans up resources and ensures all tasks are completed before destruction.
     */
    ~ThreadPoolFixed();

    /**
     * @brief Checks if the thread pool can execute a new task.
     *
     * @return `false` if the termination signal has been received or the thread pool has exited, otherwise `true`.
     */
    virtual bool executable() const override;

protected:
    /**
     * @brief Creates worker threads for the fixed thread pool.
     *
     * @param count The number of workers to create.
     */
    virtual void create_worker(std::uint32_t count) override;
};

template <class Fn, class... Args, std::enable_if_t<std::is_invocable_v<Fn, Args...>, bool> /*= true*/>
bool ThreadPool::push(Fn&& f, Args&&... args)
{
    return push(new RunnableHolder<Fn, Args...>(std::forward<Fn>(f), std::forward<Args>(args)...));
}

template <typename _Runnable, class... Args>
bool ThreadPool::emplace(Args&&... args)
{
    return push(new _Runnable(std::forward<Args>(args)...));
}

}  // namespace at

#endif
