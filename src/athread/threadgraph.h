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

#ifndef THREAD_GRAPH_H__
#define THREAD_GRAPH_H__

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <vector>

#include "node.h"
#include "noncopyable.h"
#include "task.h"
#include "worker.h"

namespace at
{

enum class WaitStatus;

enum class TraceNodeState
{
    Ready,
    Pending,
    Completed
};

/**
 * @class ThreadGraph
 * @brief A multi-threaded task execution framework based on a directed acyclic graph (DAG).
 *
 * The `ThreadGraph` class provides a robust framework for managing and executing tasks
 * represented as nodes (`INode`) in a directed acyclic graph (DAG). It ensures that tasks
 * are executed in the correct order based on their dependencies, leveraging multi-threading
 * for parallel execution where possible.
 *
 * ## Key Features:
 * - **Dependency Management**: Automatically handles task dependencies, ensuring that tasks
 *   are executed only when all their prerequisites are completed.
 * - **Multi-threading**: Supports concurrent execution of independent tasks using a configurable
 *   number of worker threads.
 * - **Task Ownership**: Takes ownership of tasks (`INode`), managing their lifecycle to prevent
 *   memory leaks or undefined behavior.
 * - **Custom Task Support**: Allows users to add custom tasks via function objects, lambdas, or
 *   derived classes of `IRunnable`.
 *
 * ## Usage:
 * 1. Create an instance of `ThreadGraph`, specifying the number of worker threads.
 * 2. Add tasks to the graph using `push` or `emplace`, defining dependencies as needed.
 * 3. Start execution with `start` and wait for completion using `wait`.
 * 4. Optionally, terminate execution prematurely with `terminate`.
 *
 * ## Thread Safety:
 * This class is non-copyable to ensure thread safety and uniqueness of the graph instance.
 * Internally, it uses mutexes and atomic variables to synchronize access to shared resources.
 *
 */

class ThreadGraph : public at::noncopyable_::noncopyable
{
    friend class IWorker;
    friend class GraphWorker;

public:
    /**
     * @brief Constructs a new ThreadGraph instance.
     * @param thread_count Number of worker threads to use (default: 2).
     * @param use_optimized_threads Whether to optimize thread usage (default: true).
     * @note The number of threads can be changed later with set_thread_count().
     */
    explicit ThreadGraph(std::uint32_t thread_count = 2, bool use_optimized_threads = true);

    /**
     * @brief Destructor. Cleans up all resources, including worker threads and tasks.
     * @details Ensures all tasks are deleted and all threads are properly joined or terminated.
     */
    virtual ~ThreadGraph();

    /**
     * @brief Adds a new task node to the graph.
     * @param node Pointer to an INode representing the task.
     * @return A Task handle representing the added node.
     * @warning Ownership of the node is transferred to ThreadGraph. Do not delete the node manually.
     */
    virtual Task push(at::INode* node);

    /**
     * @brief Adds a new task using a callable object (e.g., lambda or function).
     * @tparam Fn Type of the callable.
     * @tparam Args Types of arguments to the callable.
     * @param fn Callable object representing the task logic.
     * @param args Arguments to pass to the callable.
     * @return A Task handle representing the added node.
     */
    template <class Fn, class... Args, std::enable_if_t<std::is_invocable_v<Fn, Args...>, bool> = true>
    Task push(Fn&& fn, Args&&... args);

    /**
     * @brief Constructs and adds a new task node of type TNode to the graph.
     * @tparam TNode Type of the node to construct (must derive from INode).
     * @tparam Args Types of arguments to the node constructor.
     * @param args Arguments to pass to the node constructor.
     * @return A Task handle representing the added node.
     * @note Ownership of the node is managed by the graph.
     */
    template <class TNode, class... Args>
    Task emplace(Args&&... args);

    /**
     * @brief Removes a task from the graph.
     * @param t Task handle representing the node to remove.
     * @return true if the task was removed, false if not found.
     * @note The node is deleted and the Task handle is invalidated.
     */
    bool erase(Task& t);

    /**
     * @brief Removes all tasks from the graph, resetting it to an empty state.
     * @post All nodes are deleted and the graph is empty.
     */
    void clear();

    /**
     * @brief Starts execution of the graph using the configured number of threads.
     * @note Tasks are executed according to their dependencies.
     */
    virtual void start();

    /**
     * @brief Signals all worker threads to terminate and optionally waits for completion.
     * @param call_wait If true, waits for all threads to terminate gracefully.
     * @note Safe to call multiple times; has no effect if already terminated.
     */
    virtual void terminate(bool call_wait);

    /**
     * @brief Waits for all tasks to complete execution.
     * This is safely to call multiple times.
     * @details Blocks until all worker threads have finished processing tasks.
     */
    virtual void wait();

    /**
     * @brief Waits for all tasks to complete execution with a timeout.
     * This is safely to call multiple times.
     * @param timeout Maximum time to wait for completion.
     * @return WaitStatus indicating the result of the wait operation.
     * @note If the timeout is reached, the function returns without waiting indefinitely.
     */
    virtual WaitStatus wait_for(std::chrono::nanoseconds timeout);

    /**
     * @brief Sets the number of worker threads to use for execution.
     * @param size Desired number of threads.
     * @note Has no effect if called while executing; set before start().
     */
    void set_thread_count(std::uint32_t size) { _thread_count = size; }

    /**
     * @brief Returns the number of worker threads in use.
     * @return Number of worker threads.
     */
    std::uint32_t thread_count() const { return _thread_count; }

    /**
     * @brief Enables or disables optimized thread usage.
     * @param optimizedThreads If true, thread count is optimized based on task count.
     */
    void set_optimized_threads(bool is_optimized_threading) { _enable_optimized_threads = is_optimized_threading; }

    /**
     * @brief Returns whether optimized threading is enabled.
     * @return true if optimized threading is enabled, false otherwise.
     */
    bool optimized_threads() const { return _enable_optimized_threads; }

    /**
     * @brief Checks if the graph contains no tasks.
     * @return true if the graph is empty, false otherwise.
     */
    bool empty() const { return _task_pool.empty(); }

    /**
     * @brief Returns the number of tasks currently in the graph.
     * @return Number of tasks in the graph.
     */
    std::size_t task_size() const { return _task_pool.size(); }

    TaskIterator begin() const;
    TaskIterator end() const;
    Task task_at(std::size_t index) const;

    // Move constructor
    ThreadGraph(ThreadGraph&& other) noexcept;
    // Move assignment operator
    ThreadGraph& operator=(ThreadGraph&& other) noexcept;

private:
    /**
     * @brief Checks if the graph is currently executing tasks.
     * @return true if executing, false otherwise.
     */
    bool executing() const;

    virtual void reset_all_tasks_state();
    virtual std::pair<TraceNodeState, INode*> trace_ready_node(const INode* entryNode);
    std::pair<TraceNodeState, INode*> trace_ready_depend(
        const INode* entryNode,
        const std::unordered_set<const INode*> avoids = std::unordered_set<const INode*>()) const;
    bool remove_ready_cache(INode* node);
    virtual void create_worker(std::uint32_t count);
    std::uint32_t generate_worker_uid() const;
    virtual void reset();

    bool _enable_optimized_threads;             ///< Flag to indicate if optimized threads are used.
    std::uint32_t _thread_count;                ///< The number of worker threads to use.
    std::mutex _tasks_mutex;                    ///< Mutex for synchronizing access to tasks.
    std::atomic_bool _termination_flag{false};  ///< Signal to terminate all threads.
    std::atomic_bool _executing_flag{false};    ///< Flag to indicate if the graph is executing.
    std::condition_variable
        _task_available_condition;           ///< Condition variable for notifying workers of available tasks.
    std::vector<INode*> _task_pool;          ///< Set of tasks currently in the graph.
    std::vector<INode*> _ready_tasks_cache;  ///< Ready tasks cache for internal processing.
    std::vector<std::unique_ptr<at::WorkerContext>> _worker_contexts;  ///< Contexts for worker threads.
};

template <class Fn, class... Args, std::enable_if_t<std::is_invocable_v<Fn, Args...>, bool> /*= true*/>
Task ThreadGraph::push(Fn&& f, Args&&... args)
{
    return push(new NodeHolder<Fn, Args...>(std::forward<Fn>(f), std::forward<Args>(args)...));
}

template <class TNode, class... Args>
inline Task ThreadGraph::emplace(Args&&... args)
{
    return push(new TNode(std::forward<Args>(args)...));
}

}  // namespace at

#endif
