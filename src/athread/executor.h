//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Copyright 2025 Le Xuan Tuan Anh
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

#ifndef RUNNER_H__
#define RUNNER_H__

#include <atomic>
#include <future>

namespace at
{
class ThreadGraph;

/**
 * @class Executor
 * @brief Asynchronous executor for running ThreadGraph tasks.
 *
 * The Executor class provides a simple interface to execute a ThreadGraph asynchronously.
 * It launches the graph's execution in a separate thread and returns a std::future<void>
 * that can be used to wait for completion or catch exceptions.
 *
 * ## Usage
 * - Use Executor to run a ThreadGraph without blocking the calling thread.
 * - Retrieve the future to synchronize or check for errors.
 *
 * Example:
 * @code
 *   at::ThreadGraph graph;
 *   // ... add tasks to graph ...
 *   at::Executor executor;
 *   auto fut = executor.start(graph);
 *   // ... do other work ...
 *   fut.get(); // Wait for graph execution to finish
 * @endcode
 *
 * @note The ThreadGraph object must remain valid for the duration of the asynchronous execution.
 */
class Executor
{
public:
    /**
     * @brief Constructs a new Executor instance.
     */
    Executor();

    /**
     * @brief Destructor.
     */
    virtual ~Executor();

    /**
     * @brief Starts execution of the given ThreadGraph asynchronously.
     *
     * Launches the graph's execution in a new detached thread. The returned future
     * becomes ready when all tasks in the graph have completed or if an exception occurs.
     *
     * @param graph Reference to the ThreadGraph to execute.
     * @return std::future<void> that can be used to wait for completion or catch exceptions.
     * @note The graph must remain valid until the future is ready.
     */
    std::future<void> start(at::ThreadGraph& graph);

    /**
     * @brief Starts execution of the given ThreadGraph multiple times asynchronously.
     *
     * Repeatedly starts and waits for the graph execution for the specified number of times,
     * each in sequence, in a new detached thread. The returned future becomes ready when all
     * iterations have completed or if an exception occurs.
     *
     * @param graph Reference to the ThreadGraph to execute.
     * @param times Number of times to execute the graph.
     * @return std::future<void> that can be used to wait for completion or catch exceptions.
     * @note The graph must remain valid until the future is ready.
     */
    std::future<void> start_loop(at::ThreadGraph& graph, std::size_t times);
};

}  // namespace at

#endif  // RUNNER_H__