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

#ifndef TASK_H__
#define TASK_H__

#include <memory>
#include <optional>
#include <vector>

#include "node.h"

namespace at
{

class TaskIterator;
/**
 * @class Task
 * @brief Represents a handle to a task node in the ThreadGraph.
 *
 * The Task class is a lightweight, non-owning handle to an INode managed by a ThreadGraph.
 * It provides methods to define task dependencies, query relationships, and compare tasks.
 *
 * @note The lifetime of the underlying INode is managed by ThreadGraph, not by Task.
 * @note Copying or assigning a Task only copies the handle; it does not duplicate the node.
 */
class Task
{
    friend class ThreadGraph;
    friend class TaskIterator;

public:
    /**
     * @enum TaskState
     * @brief Represents the execution state of a task.
     */
    enum TaskState
    {
        READY,      ///< Task is ready to execute.
        EXECUTING,  ///< Task is currently executing.
        COMPLETED   ///< Task execution is complete.
    };

    /**
     * @brief Default constructor. Constructs an invalid Task handle.
     *
     * The constructed Task does not reference any node.
     */
    Task();

    /**
     * @brief Copy constructor.
     *
     * Copies the handle to the same underlying INode as the original.
     * No new node is created.
     *
     * @param other The Task to copy from.
     */
    Task(const Task& other) = default;

    /**
     * @brief Copy assignment operator.
     *
     * Assigns this Task to reference the same INode as another Task.
     * No new node is created.
     *
     * @param other The Task to assign from.
     * @return Reference to this Task.
     */
    Task& operator=(const Task& other) = default;

    /**
     * @brief Adds a dependency: this task will execute after the given task.
     *
     * @param other The Task that must complete before this one starts.
     * @return Reference to this Task.
     * @note No effect if either task is invalid.
     */
    Task& depend(const Task& other);

    /**
     * @brief Adds dependencies: this task will execute after all given tasks.
     *
     * @param others Vector of Tasks that must complete before this one starts.
     * @return Reference to this Task.
     * @note No effect for invalid tasks in the vector.
     */
    Task& depend(const std::vector<Task>& others);

    /**
     * @brief Adds a dependency: the given task will execute after this task.
     *
     * @param other The Task that will execute after this one.
     * @return Reference to this Task.
     * @note No effect if either task is invalid.
     */
    Task& precede(const Task& other);

    /**
     * @brief Adds dependencies: all given tasks will execute after this task.
     *
     * @param others Vector of Tasks that will execute after this one.
     * @return Reference to this Task.
     * @note No effect for invalid tasks in the vector.
     */
    Task& precede(const std::vector<Task>& others);

    /**
     * @brief Removes a dependency: this task will no longer depend on the given task.
     *
     * @param other The Task to remove as a dependency.
     * @return Reference to this Task.
     * @note No effect if either task is invalid or not dependent.
     */
    Task& erase_depend(const Task& other);

    /**
     * @brief Removes dependencies: this task will no longer depend on the given tasks.
     *
     * @param others Vector of Tasks to remove as dependencies.
     * @return Reference to this Task.
     * @note No effect for invalid or non-dependent tasks.
     */
    Task& erase_depend(const std::vector<Task>& others);

    /**
     * @brief Removes a dependency: the given task will no longer depend on this task.
     *
     * @param other The Task to remove as a dependent.
     * @return Reference to this Task.
     * @note No effect if either task is invalid or not a dependent.
     */
    Task& erase_precede(const Task& other);

    /**
     * @brief Removes dependencies: the given tasks will no longer depend on this task.
     *
     * @param others Vector of Tasks to remove as dependents.
     * @return Reference to this Task.
     * @note No effect for invalid or non-dependent tasks.
     */
    Task& erase_precede(const std::vector<Task>& others);

    /**
     * @brief Checks if two Task handles refer to the same underlying node.
     *
     * @param other The Task to compare with.
     * @return true if both refer to the same INode, false otherwise.
     */
    bool operator==(const Task& other) const;

    /**
     * @brief Checks if two Task handles refer to different nodes.
     *
     * @param other The Task to compare with.
     * @return true if they refer to different INode objects, false otherwise.
     */
    bool operator!=(const Task& other) const;

    /**
     * @brief Gets the current execution state of the task.
     *
     * @return The TaskState (READY, EXECUTING, or COMPLETED).
     * @note Returns READY if the handle is invalid.
     */
    TaskState state() const { return static_cast<TaskState>(_node->state()); }

    /**
     * @brief Resets the state of the underlying node to READY.
     *
     * @note No effect if the handle is invalid.
     */
    void reset_state();

    bool empty() const { return _node == nullptr; }

    /**
     * @brief Returns the number of predecessor (dependency) tasks.
     *
     * @return Number of tasks that must complete before this one.
     */
    std::size_t predecessors_size() const { return _node->_predecessors.size(); }

    /**
     * @brief Returns the number of successor (dependent) tasks.
     *
     * @return Number of tasks that depend on this one.
     */
    std::size_t successors_size() const { return _node->_successors.size(); }

    /**
     * @brief Returns an iterator to the beginning of the predecessor set.
     *
     * @return TaskIterator to the first predecessor.
     */
    TaskIterator begin_predecessors() const;

    /**
     * @brief Returns an iterator to the end of the predecessor set.
     *
     * @return TaskIterator to one past the last predecessor.
     */
    TaskIterator end_predecessors() const;

    /**
     * @brief Returns an iterator to the beginning of the successor set.
     *
     * @return TaskIterator to the first successor.
     */
    TaskIterator begin_successors() const;

    /**
     * @brief Returns an iterator to the end of the successor set.
     *
     * @return TaskIterator to one past the last successor.
     */
    TaskIterator end_successors() const;

    Task predecessor_at(std::size_t index) const { return Task(_node->_predecessors.at(index)); }
    Task successor_at(std::size_t index) const { return Task(_node->_successors.at(index)); }

private:
    /**
     * @brief Constructs a Task handle for a given node.
     *
     * Used internally by ThreadGraph.
     *
     * @param node Pointer to the associated INode (may be nullptr for invalid Task).
     */
    explicit Task(INode* node) : _node{node} {}

    INode* _node;  ///< Pointer to the associated INode object (may be nullptr for invalid Task).
};

class TaskIterator
{
public:
    explicit TaskIterator(std::vector<INode*>::const_iterator it) : _it(it), _cached_task(nullptr) {}

    Task operator*() const { return Task(*_it); }
    Task* operator->() const;

    TaskIterator& operator++();

    TaskIterator operator++(int);

    bool operator!=(const TaskIterator& other) const { return _it != other._it; }
    bool operator==(const TaskIterator& other) const { return _it == other._it; }

private:
    std::vector<INode*>::const_iterator _it;  // Underlying iterator
    mutable Task _cached_task;                // Cached Task object
};

}  // namespace at

#endif  // TASK_H__