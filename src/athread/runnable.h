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

#ifndef RUNNABLE_H__
#define RUNNABLE_H__

#include <atomic>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_set>

namespace at
{

/**
 * @class IRunnable
 * @brief Abstract interface for runnable tasks in a thread pool.
 *
 * The `IRunnable` class provides a base interface for defining tasks that can
 * be executed by a thread pool. It tracks the state of the task during its lifecycle
 * and defines a pure virtual `execute` method to be implemented by derived classes.
 *
 * This class is designed for multi-threaded environments and uses an atomic state
 * to ensure thread safety.
 */
class IRunnable
{
    friend class GraphWorker;
    friend class ThreadPoolWorker;
    friend class ThreadSeasonalWorker;
    friend class ThreadGraph;
    friend class INode;
    friend class Task;

public:
    /**
     * @enum STATE
     * @brief Represents the possible states of a task.
     *
     * - `READY`: The task is ready to be executed.
     * - `EXECUTING`: The task is currently being executed by a thread.
     * - `COMPLETE`: The task has finished execution.
     */
    enum RunnableState
    {
        Ready,      // Task is ready to execute.
        Executing,  // Task is currently executing.
        Completed   // Task execution is complete.
    };

    /**
     * @brief Constructor.
     *
     * Initializes the task state to `STATE::READY`.
     */
    IRunnable() { _state = RunnableState::Ready; };

    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived classes.
     */
    virtual ~IRunnable() {};

    /**
     * @brief Get the current state of the task.
     *
     * @return The current state of the task as an integer (`STATE` enum value).
     */
    int state() const { return _state.load(); }

    /**
     * @brief Get identify of a runnable.
     * @return The indentify string. Memory location as string is default.
     */
    virtual std::string id() const;

    static std::string state_to_string(int state);

protected:
    /**
     * @brief Pure virtual method to define task execution logic.
     *
     * Derived classes must override this method to implement custom task behavior.
     *
     * @note This method is intended to be invoked by the thread pool.
     */
    virtual void execute() = 0;

private:
    virtual void set_state(int state) { _state.store(state); };

    std::atomic_int _state;  // Atomic variable to track the state of the task.
};

inline std::string IRunnable::id() const
{
    std::ostringstream oss;
    oss << static_cast<const void*>(this);
    return oss.str();
}

inline std::string IRunnable::state_to_string(int state)
{
    if (state == RunnableState::Ready) return "Ready";
    if (state == RunnableState::Executing) return "Executing";
    if (state == RunnableState::Completed) return "Completed";
    return "";
}

/**
 * @class RunnableHolder
 * @brief Template class for wrapping callable objects and their arguments as runnable tasks.
 *
 * The `RunnableHolder` class is a concrete implementation of the `IRunnable` interface.
 * It allows any callable object (e.g., function pointers, lambdas, or function objects)
 * along with its arguments to be encapsulated and executed as a task in a thread pool.
 *
 * @tparam Fn The type of the callable object.
 * @tparam Args The types of the arguments for the callable object.
 */
template <class Fn, class... Args>
class RunnableHolder : public IRunnable
{
public:
    /**
     * @brief Constructor.
     *
     * Initializes the `RunnableHolder` with a callable object and its arguments.
     *
     * @param fn A callable object (e.g., function pointer, lambda, or function object).
     * @param args The arguments to be passed to the callable object during execution.
     *
     * @note Arguments are stored in a tuple internally and forwarded to the callable
     *       object during execution.
     */
    explicit RunnableHolder(Fn&& fn, Args&&... args);

protected:
    /**
     * @brief Executes the encapsulated callable object with its arguments.
     *
     * This method overrides `IRunnable::execute` and applies the stored arguments
     * to the callable object using `std::apply`. It is intended to be called by
     * the thread pool or task executor.
     */
    void execute() { std::apply(_func, _params); };

    Fn _func;                     // The callable object to be executed.
    std::tuple<Args...> _params;  // The arguments for the callable object, stored as a tuple.
};

template <class Fn, class... Args>
at::RunnableHolder<Fn, Args...>::RunnableHolder(Fn&& fn, Args&&... args)
    : _func(std::forward<Fn>(fn)), _params(std::forward<Args>(args)...)
{
}

}  // namespace at

#endif  // RUNNABLE_H__