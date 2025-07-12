//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Copyright 2024 Le Xuan Tuan Anh
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

#ifndef WORKER_H__
#define WORKER_H__

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <queue>
#include <thread>

#include "noncopyable.h"

namespace at
{

class ThreadPool;
class ThreadGraph;

/**
 * @enum WorkerState
 * @brief Represents the possible states of a worker.
 *
 * - `Ready`: The worker is waiting for tasks to be assigned.
 * - `Delay`: The worker is waiting for a start signal.
 * - `Busy`: The worker is actively executing a task.
 * - `Completed`: The worker has completed its work and will terminate.
 */
enum class WorkerState
{
    Ready,
    Delay,
    Busy,
    Completed
};

/**
 * @class Worker
 * @brief Represents a worker thread that executes tasks in a thread pool or thread graph.
 *
 * The `Worker` class is responsible for executing tasks provided by either a `ThreadPool` or
 * a `ThreadGraph`. It manages its state and provides methods to process tasks based on
 * different conditions, such as waiting for a start signal or running for a limited duration.
 */
class IWorker : public at::noncopyable_::noncopyable
{
public:
    /**
     * @brief Constructs a `Worker` with a unique identifier.
     *
     * This constructor is protected and intended to be used by derived classes or friend classes.
     *
     * @param id A unique identifier for the worker.
     */
    IWorker(std::uint32_t id);

    IWorker(IWorker&& other) noexcept : _id(other._id), _done(std::move(other._done)), _state(other._state.load()) {}

    IWorker& operator=(IWorker&& other) noexcept
    {
        if (this != &other)
        {
            _id = other._id;
            _done = std::move(other._done);
            _state.store(other._state.load());
        }
        return *this;
    }
    /**
     * @brief Destructor.
     *
     * Cleans up the worker instance. Derived classes should override as needed.
     */
    virtual ~IWorker() {};

    std::future<void> get_future() { return _done.get_future(); }
    std::thread::id thread_id() { return std::this_thread::get_id(); }
    WorkerState state() const { return _state.load(); }

    virtual void process_tasks() = 0;  // Pure virtual function to be implemented by derived classes.

protected:
    std::uint32_t _id;                                    // A unique identifier for the worker.
    std::promise<void> _done;                             // Promise to signal when the worker has completed its tasks.
    std::atomic<WorkerState> _state{WorkerState::Ready};  // Current state of the worker.
};

class ThreadPoolWorker : public IWorker
{
public:
    /**
     * @brief Constructs a `PoolWorker` with a unique identifier.
     *
     * This constructor is protected and intended to be used by derived classes or friend classes.
     *
     * @param id A unique identifier for the worker.
     */
    ThreadPoolWorker(std::uint32_t id, at::ThreadPool* pool) : IWorker(id), _pool(pool) {}

    /**
     * @brief Waits for a start signal to begin task execution.
     *
     * This method is used to synchronize the worker with the `ThreadPool` start signal.
     *
     * @param pool The `ThreadPool` instance managing this worker.
     */
    void await_start_signal();

    /**
     * @brief Executes tasks in a thread pool until termination is signaled.
     *
     * This method processes tasks in the thread pool based on task dependencies.
     *
     * @param pool The `ThreadPool` instance managing this worker.
     */
    virtual void process_tasks() override;

protected:
    at::ThreadPool* _pool;  // Reference to the thread pool this worker is associated with.
};

class ThreadSeasonalWorker : public ThreadPoolWorker
{
public:
    /**
     * @brief Constructs a `SeasonalWorker` with a unique identifier.
     *
     * This constructor is protected and intended to be used by derived classes or friend classes.
     *
     * @param id A unique identifier for the worker.
     */
    ThreadSeasonalWorker(std::uint32_t id, at::ThreadPool* pool, const std::chrono::nanoseconds& alive_time)
        : ThreadPoolWorker(id, pool), _alive_duration(alive_time)
    {
    }
    virtual void process_tasks() override;  // Override to provide specific seasonal worker behavior.

protected:
    std::chrono::nanoseconds _alive_duration;  // Duration for which the worker will remain active.
};

class GraphWorker : public IWorker
{
public:
    /**
     * @brief Constructs a `WorkerGraph` with a unique identifier.
     *
     * This constructor is protected and intended to be used by derived classes or friend classes.
     *
     * @param id A unique identifier for the worker graph.
     */
    GraphWorker(std::uint32_t id, at::ThreadGraph* graph) : IWorker(id), _graph(graph) {}
    /**
     * @brief Executes tasks in a `ThreadGraph` until termination is signaled.
     *
     * This method processes tasks in the `ThreadGraph` based on task dependencies.
     *
     * @param graph The `ThreadGraph` instance managing this worker.
     */
    virtual void process_tasks() override;

protected:
    at::ThreadGraph* _graph;  // Reference to the thread graph this worker is associated with.
};

struct WorkerContext
{
    std::unique_ptr<at::IWorker> worker;
    std::thread thread;
    std::future<void> future;
};

}  // namespace at

#endif  // WORKER_H__