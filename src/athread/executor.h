//////////////////////////////////////////////////////////////////////////
// Copyright 2025 Le Xuan Tuan Anh
//
// https://github.com/z-pc/athread
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

#ifndef EXECUTOR_H__
#define EXECUTOR_H__

#include <atomic>
#include <functional>
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
    Executor() {};
    virtual ~Executor() {};

    /**
     * @brief General start function.
     *
     * Execute graph repeatedly. After each run, check stop_condition.
     * Stop when stop_condition() is true OR max_times is reached.
     * Then invoke callback() once.
     */
    std::future<void> start(at::ThreadGraph& graph, std::size_t max_times, const std::function<bool()>& stop_condition,
                            const std::function<void()>& callback);

    /**
     * @brief Execute once.
     */
    std::future<void> start(at::ThreadGraph& graph);

    /**
     * @brief Execute fixed number of times.
     */
    std::future<void> start(at::ThreadGraph& graph, std::size_t times);

    /**
     * @brief Execute fixed number of times + callback, no stop condition.
     */
    std::future<void> start(at::ThreadGraph& graph, std::size_t times, const std::function<void()>& callback);

    /**
     * @brief Execute once, with callback.
     */
    std::future<void> start(at::ThreadGraph& graph, const std::function<void()>& callback);

    /**
     * @brief Execute graph repeatedly. After each run, check stop_condition.
     * Stop when stop_condition() is true OR max_times is reached.
     */
    std::future<void> start_until(at::ThreadGraph& graph, std::size_t max_times,
                                  const std::function<bool()>& stop_condition);

    /**
     * @brief Execute until stop condition, with callback.
     */
    std::future<void> start_until(at::ThreadGraph& graph, const std::function<bool()>& stop_condition,
                                  const std::function<void()>& callback);

    /**
     * @brief Execute until stop condition, no callback.
     */
    std::future<void> start_until(at::ThreadGraph& graph, const std::function<bool()>& stop_condition);
};

}  // namespace at

#endif  // EXECUTOR_H__