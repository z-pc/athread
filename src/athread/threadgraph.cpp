#include "threadgraph.h"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>

#include "athread.h"
#include "worker.h"

using namespace at;
using namespace std;

void ThreadGraph::create_worker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        std::unique_ptr<WorkerContext> context = std::make_unique<WorkerContext>();
        context->worker.reset(new GraphWorker(generate_worker_uid(), this));
        context->future = context->worker->get_future();
        context->thread = std::thread(&IWorker::process_tasks, context->worker.get());
        _worker_contexts.push_back(std::move(context));
    }
}

ThreadGraph::ThreadGraph(std::uint32_t thread_count, bool enable_optimized_threads)
{
    _thread_count = thread_count;
    _termination_flag = false;
    _enable_optimized_threads = enable_optimized_threads;
}

ThreadGraph::~ThreadGraph() { clear(); }

at::Task ThreadGraph::push(at::INode* node)
{
    if (node == nullptr) AT_INVALID_ARGUMENT("Node is null. A valid node must be provided.");
    if (executing()) AT_RUNTIME_ERROR("Cannot push tasks while executing.");

    if (node->state() == INode::Executing || node->state() == INode::Completed)
        AT_INVALID_ARGUMENT("Node is already in EXECUTING or COMPLETE state. A valid node must be provided.");

    // Check if the node is already in the graph
    if (std::find(_task_pool.begin(), _task_pool.end(), node) != _task_pool.end())
        AT_INVALID_ARGUMENT("Node is already in the graph. A valid node must be provided.");

    // Insert the node into the graph
    // The node is not already in the graph, so we can safely insert it.
    _task_pool.push_back(node);
    return Task(node);
}

bool at::ThreadGraph::erase(Task& t)
{
    if (t._node == nullptr) return false;
    if (executing()) AT_RUNTIME_ERROR("Cannot erase tasks while executing.");

    auto it = std::find(_task_pool.begin(), _task_pool.end(), t._node);
    if (it == _task_pool.end()) return false;

    for (auto* predecessor : t._node->_predecessors)
    {
        predecessor->_successors.erase(
            std::remove(predecessor->_successors.begin(), predecessor->_successors.end(), t._node),
            predecessor->_successors.end());
    }

    for (auto* successor : t._node->_successors)
    {
        successor->_predecessors.erase(
            std::remove(successor->_predecessors.begin(), successor->_predecessors.end(), t._node),
            successor->_predecessors.end());
    }

    _task_pool.erase(it);
    delete t._node;
    t._node = nullptr;

    return true;
}

void ThreadGraph::clear()
{
    // if (executing()) AT_RUNTIME_ERROR("Cannot clear tasks while executing.");

    reset();
    for (auto t : _task_pool) delete t;
    _task_pool.clear();
}

void ThreadGraph::reset_all_tasks_state()
{
    std::lock_guard<std::mutex> lk{_tasks_mutex};

    for (INode* t : _task_pool) t->set_state(INode::Ready);
}

void at::ThreadGraph::start()
{
    if (executing()) AT_RUNTIME_ERROR("Cannot start execution while already executing.");

    // Wait for all threads to finish before starting new tasks. Ensure that the graph is not executing.
    wait();
    reset();
    reset_all_tasks_state();
    _executing_flag.store(true);
    _ready_tasks_cache = _task_pool;

    uint32_t numThreads = _thread_count;

    // If optimized threads are enabled, adjust the number of threads based on the number of tasks.
    if (_enable_optimized_threads)
    {
        numThreads = std::min(_thread_count, static_cast<std::uint32_t>(_task_pool.size()));
    }
    create_worker(numThreads);
}

void at::ThreadGraph::terminate(bool call_wait /*= true*/)
{
    _termination_flag.store(true);
    if (call_wait) wait();
}

void at::ThreadGraph::wait()
{
    std::string exception_msg;

    for (auto& context : _worker_contexts)
    {
        if (!context || !context->future.valid()) continue;

        try
        {
            context->future.get();  // Wait for each worker to finish.
        }
        catch (const std::exception& e)
        {
            exception_msg += e.what();
            exception_msg += "\n";  // Collect exception messages from all workers.
        }
    }

    for (auto& context : _worker_contexts)
    {
        if (context && context->thread.joinable())
        {
            context->thread.join();
        }
    }

    reset();

    if (!exception_msg.empty())
    {
        throw std::runtime_error("Exception occurred in worker thread: " + exception_msg);
    }
}

at::ThreadGraph& ThreadGraph::operator=(ThreadGraph&& other) noexcept
{
    if (this != &other)
    {
        std::lock(_tasks_mutex, other._tasks_mutex);
        std::lock_guard<std::mutex> lhs_lock(_tasks_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> rhs_lock(other._tasks_mutex, std::adopt_lock);

        _enable_optimized_threads = other._enable_optimized_threads;
        _thread_count = other._thread_count;
        _task_pool = std::move(other._task_pool);
        _ready_tasks_cache = std::move(other._ready_tasks_cache);
        _worker_contexts = std::move(other._worker_contexts);

        _termination_flag.store(other._termination_flag.load());
        _executing_flag.store(other._executing_flag.load());
        // No need to move mutex or condition_variable, just leave as is
    }
    return *this;
}

ThreadGraph::ThreadGraph(ThreadGraph&& other) noexcept
    : _enable_optimized_threads(other._enable_optimized_threads),
      _thread_count(other._thread_count),
      _task_pool(std::move(other._task_pool)),
      _ready_tasks_cache(std::move(other._ready_tasks_cache)),
      _worker_contexts(std::move(other._worker_contexts))
{
    // Atomics and condition_variable cannot be moved, so reset them
    _termination_flag.store(other._termination_flag.load());
    _executing_flag.store(other._executing_flag.load());
    // No need to move mutex or condition_variable, just default construct
}

WaitStatus ThreadGraph::wait_for(std::chrono::nanoseconds timeout)
{
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now() + timeout;
    std::chrono::steady_clock::duration remaining_time = timeout;

    for (auto& context : _worker_contexts)
    {
        if (!context || !context->future.valid()) continue;

        if (context->future.wait_for(remaining_time) == std::future_status::timeout)
        {
            return WaitStatus::Timeout;  // If any future times out, return timeout status.
        }

        // If the future is ready, we can continue to the next one.
        remaining_time = end_time - std::chrono::steady_clock::now();
        if (remaining_time <= std::chrono::steady_clock::duration::zero())
        {
            return WaitStatus::Timeout;  // If remaining time is zero or negative, return timeout status.
        }
    }

    // If we reach here, it means all futures are ready or completed within the timeout.
    // We can safely call wait() to ensure all threads have completed execution.
    wait();

    // If we successfully waited for all futures, return ready status.
    return WaitStatus::Ready;
}

at::TaskIterator ThreadGraph::begin() const { return TaskIterator(_task_pool.begin()); }

at::TaskIterator ThreadGraph::end() const { return TaskIterator(_task_pool.end()); }

at::Task ThreadGraph::task_at(std::size_t index) const { return Task(_task_pool[index]); }

void ThreadGraph::reset()
{
    _executing_flag.store(false);
    _termination_flag.store(false);
    _ready_tasks_cache.clear();
    _worker_contexts.clear();
}

bool ThreadGraph::executing() const { return _executing_flag.load(); }

bool ThreadGraph::remove_ready_cache(INode* node)
{
    auto it = std::find(_ready_tasks_cache.begin(), _ready_tasks_cache.end(), node);
    if (it != _ready_tasks_cache.end())
    {
        _ready_tasks_cache.erase(it);
        return true;
    }
    return false;
}

std::pair<at::TraceNodeState, INode*> ThreadGraph::trace_ready_depend(
    const INode* entryNode,
    const std::unordered_set<const INode*> avoids /*= std::unordered_set<const INode*>()*/) const
{
    if (entryNode == nullptr) AT_RUNTIME_ERROR("EntryNode is null. A valid entry node must be provided.");

    if (entryNode->state() == INode::RunnableState::Executing)
    {
        return std::pair(TraceNodeState::Pending, const_cast<INode*>(entryNode));
    }
    else if (entryNode->state() == INode::RunnableState::Completed)
    {
        return std::pair(TraceNodeState::Completed, const_cast<INode*>(entryNode));
    }
    else
    {
        std::pair<at::TraceNodeState, INode*> exe;
        for (const auto precede : entryNode->_predecessors)
        {
            if (precede == nullptr) continue;  // Skip null predecessors.
            if (avoids.size() > 0 && avoids.find(precede) != avoids.end()) continue;

            if (precede->state() == INode::Ready)
            {
                auto branch_state = trace_ready_depend(precede, avoids);
                if (branch_state.first == TraceNodeState::Ready)
                {
                    return branch_state;
                }
                else if (branch_state.first == TraceNodeState::Pending)
                {
                    exe = branch_state;
                }
            }
            else if (precede->state() == INode::Executing)
            {
                exe = std::pair(TraceNodeState::Pending, precede);
            }
        }

        if (exe.second)
            return exe;
        else
            return std::pair(TraceNodeState::Ready, const_cast<INode*>(entryNode));
    }
}

std::uint32_t ThreadGraph::generate_worker_uid() const { return _worker_contexts.size(); }

std::pair<at::TraceNodeState, INode*> ThreadGraph::trace_ready_node(const INode* entryNode /*= nullptr*/)
{
    if (entryNode == nullptr)
    {
        if (_ready_tasks_cache.size() > 0) return trace_ready_depend(_ready_tasks_cache.at(0));

        for (const auto task : _task_pool)
            if (task->state() == INode::Executing) return std::pair(TraceNodeState::Pending, task);
    }
    else
    {
        if (entryNode->state() == INode::Executing)
        {
            // If the entry node is executing, then the succeeds should be ready.

            for (auto node : entryNode->_successors)
            {
                if (node->state() == INode::Ready)
                {
                    auto trackingnode_ = trace_ready_depend(node);
                    if (trackingnode_.first == at::TraceNodeState::Ready)
                    {
                        return trackingnode_;
                    }
                }
            }

            auto next_ready_task = trace_ready_node(nullptr);
            if (next_ready_task.first == TraceNodeState::Ready)
            {
                return next_ready_task;
            }

            return std::pair(TraceNodeState::Pending, const_cast<INode*>(entryNode));
        }
        else if (entryNode->state() == INode::Ready)
        {
            auto ready_depend = trace_ready_depend(entryNode);
            if (ready_depend.first == TraceNodeState::Ready) return ready_depend;

            if (ready_depend.first == TraceNodeState::Pending)
            {
                auto next_ready_task = trace_ready_node(nullptr);
                if (next_ready_task.first == TraceNodeState::Ready)
                {
                    return next_ready_task;
                }

                return ready_depend;
            }
        }
        else if (entryNode->state() == INode::Completed)
        {
            std::pair<at::TraceNodeState, INode*> delay(TraceNodeState::Pending, nullptr);

            for (auto node : entryNode->_successors)
            {
                if (node->state() == INode::Ready)
                {
                    auto trackingnode_ = trace_ready_depend(node);
                    if (trackingnode_.first == at::TraceNodeState::Ready)
                    {
                        return trackingnode_;
                    }
                    else if (trackingnode_.first == TraceNodeState::Pending)
                    {
                        delay = trackingnode_;
                    }
                }
            }

            auto readynode_ = trace_ready_node(nullptr);
            if (readynode_.first == TraceNodeState::Ready) return readynode_;
            if (delay.second) return delay;
            if (readynode_.first == TraceNodeState::Pending) return readynode_;
        }
    }

    return std::pair(TraceNodeState::Completed, nullptr);
}