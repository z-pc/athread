#include "task.h"

#include <algorithm>

#include "athread.h"

using namespace at;

Task::Task() { _node = nullptr; }

Task& at::Task::depend(const Task& t)
{
    if (t._node == nullptr) AT_INVALID_ARGUMENT("Task is not valid");
    if (t._node == _node) AT_INVALID_ARGUMENT("Cannot set relation to itself");

    // Check to avoid circular dependency
    if (std::find(t._node->_predecessors.begin(), t._node->_predecessors.end(), this->_node) !=
        t._node->_predecessors.end())
        AT_RUNTIME_ERROR("Circular dependency detected");

    if (std::find(_node->_predecessors.begin(), _node->_predecessors.end(), t._node) == _node->_predecessors.end())
    {
        _node->_predecessors.push_back(t._node);
    }
    if (std::find(t._node->_successors.begin(), t._node->_successors.end(), this->_node) == t._node->_successors.end())
    {
        t._node->_successors.push_back(this->_node);
    }

    return *this;
}

at::Task& Task::depend(const std::vector<Task>& others)
{
    for (const auto& t : others) depend(t);
    return *this;
}

Task& at::Task::precede(const Task& t)
{
    Task tmp = t;
    tmp.depend(*this);
    return *this;
}

at::Task& Task::precede(const std::vector<Task>& others)
{
    for (const auto& t : others) precede(t);
    return *this;
}

Task& Task::erase_depend(const Task& other)
{
    if (_node && other._node)
    {
        _node->_predecessors.erase(std::remove(_node->_predecessors.begin(), _node->_predecessors.end(), other._node),
                                   _node->_predecessors.end());
        other._node->_successors.erase(
            std::remove(other._node->_successors.begin(), other._node->_successors.end(), _node),
            other._node->_successors.end());
    }
    return *this;
}

Task& Task::erase_depend(const std::vector<Task>& others)
{
    for (const auto& other : others)
    {
        erase_depend(other);
    }
    return *this;
}

Task& Task::erase_precede(const Task& other)
{
    if (_node && other._node)
    {
        _node->_successors.erase(std::remove(_node->_successors.begin(), _node->_successors.end(), other._node),
                                 _node->_successors.end());
        other._node->_predecessors.erase(
            std::remove(other._node->_predecessors.begin(), other._node->_predecessors.end(), _node),
            other._node->_predecessors.end());
    }
    return *this;
}

Task& Task::erase_precede(const std::vector<Task>& others)
{
    for (const auto& other : others)
    {
        erase_precede(other);
    }
    return *this;
}

bool Task::operator!=(const Task& other) const { return _node != other._node; }

bool Task::operator==(const Task& other) const { return _node == other._node; }

void at::Task::reset_state()
{
    if (_node) _node->set_state(INode::Ready);
}

at::TaskIterator Task::begin_predecessors() const
{
    return _node ? TaskIterator(_node->_predecessors.begin()) : TaskIterator({});
}

at::TaskIterator Task::end_predecessors() const
{
    return _node ? TaskIterator(_node->_predecessors.end()) : TaskIterator({});
}

at::TaskIterator Task::begin_successors() const
{
    return _node ? TaskIterator(_node->_successors.begin()) : TaskIterator({});
}

at::TaskIterator Task::end_successors() const
{
    return _node ? TaskIterator(_node->_successors.end()) : TaskIterator({});
}

at::TaskIterator TaskIterator::operator++(int)
{
    TaskIterator temp = *this;
    ++(*this);
    return temp;
}

at::TaskIterator& TaskIterator::operator++()
{
    ++_it;
    _cached_task._node = nullptr;
    return *this;
}

at::Task* TaskIterator::operator->() const
{
    if (_cached_task._node != *_it)
    {
        _cached_task._node = *_it;
    }
    return &_cached_task;
}
