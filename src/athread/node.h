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

#ifndef NODE_H__
#define NODE_H__

#include "runnable.h"

namespace at
{

/**
 * @class INode
 * @brief Represents a unit of execution in a dependency graph for threaded tasks.
 *
 * The `INode` class extends the `IRunnable` interface and serves as a building block
 * for representing tasks in a directed acyclic graph (DAG). Each `INode` maintains
 * its dependencies and dependents, enabling execution based on task dependencies.
 *
 * @note This class is primarily intended for use within a thread graph executor.
 */
class INode : public IRunnable
{
    friend class Task;
    friend class ThreadGraph;

public:
    const std::vector<INode*>& predecessors() const { return _predecessors; }
    const std::vector<INode*>& successors() const { return _successors; }

protected:
    using IRunnable::IRunnable;

private:
    std::vector<INode*> _predecessors;  ///< Set of predecessor nodes (dependencies).
    std::vector<INode*> _successors;    ///< Set of successor nodes (dependents).
};

/**
 * @class NodeHolder
 * @brief Template class for creating callable-based `Node` objects.
 *
 * The `NodeHolder` class extends `Node` and encapsulates a callable object along
 * with its arguments, allowing it to represent a task in a dependency graph.
 *
 * @tparam Fn The type of the callable object.
 * @tparam Args The types of the arguments for the callable object.
 */
template <class Fn, class... Args>
class NodeHolder : public INode
{
public:
    /**
     * @brief Constructor.
     *
     * Initializes the `NodeHolder` with a callable object and its arguments.
     *
     * @param fn A callable object (e.g., function pointer, lambda, or function object).
     * @param args The arguments to be passed to the callable object during execution.
     *
     * @note Arguments are stored in a tuple internally and forwarded to the callable
     *       object during execution.
     */
    explicit NodeHolder(Fn&& fn, Args&&... args);

protected:
    void execute() { std::apply(_func, _params); };

private:
    Fn _func;                     // The callable object to be executed.
    std::tuple<Args...> _params;  // The arguments for the callable object, stored as a tuple.
};

template <class Fn, class... Args>
at::NodeHolder<Fn, Args...>::NodeHolder(Fn&& fn, Args&&... args)
    : _func(std::forward<Fn>(fn)), _params(std::forward<Args>(args)...)
{
}

}  // namespace at

#endif  // NODE_H__