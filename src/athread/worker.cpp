#include "worker.h"

#include "athread.h"
#include "threadgraph.h"
#include "threadpool.h"

using namespace at;
using namespace std;

IWorker::IWorker(std::uint32_t id)
{
    _id = id;
    _state.store(WorkerState::Delay);
    AT_LOG("create worker " << id);
}

// void Worker::set_state(int state) { state_.store(state); }

void ThreadPoolWorker::await_start_signal()
{
    std::unique_lock<std::mutex> lk{_pool->_task_queue_mutex};
    AT_LOG("worker " << this->_id << " is waiting for start signal");
    _pool->_work_available_condition.wait(lk, [&]() { return !_pool->_wait_for_start_signal.load(); });
    lk.unlock();
}

void at::GraphWorker::process_tasks()
try
{
    if (!_graph) AT_ERROR("ThreadGraph is not initialized.");

    _state.store(WorkerState::Busy);
    std::pair<at::TraceNodeState, INode*> nextNode(at::TraceNodeState::Pending, nullptr);

    do
    {
        {
            if (_graph->_termination_flag.load()) break;

            std::unique_lock<std::mutex> lk{_graph->_tasks_mutex};
            nextNode = _graph->trace_ready_node(nextNode.second);

            if (nextNode.second) AT_LOG("worker " << this->_id << " is considering a task: " << nextNode.second->id());

            if (nextNode.first == at::TraceNodeState::Ready)
            {
                nextNode.second->set_state(IRunnable::Executing);
                _graph->remove_ready_cache(nextNode.second);
            }
            else if (nextNode.first == at::TraceNodeState::Pending)
            {
                _graph->_task_available_condition.wait(lk);
            }
        }

        if (nextNode.first == at::TraceNodeState::Ready)
        {
            if (nextNode.second)
            {
                nextNode.second->execute();
                nextNode.second->set_state(IRunnable::Completed);

                _graph->_task_available_condition.notify_all();
            }
        }
        else if (nextNode.first == at::TraceNodeState::Completed)
        {
            break;
        }

    } while (true);

    _graph->_task_available_condition.notify_all();
    AT_LOG("worker " << this->_id << " is exited");
    _state.store(WorkerState::Completed);
    _done.set_value();
}
catch (...)
{
    // stop graph execution if an exception occurs
    if (_graph)
    {
        _graph->_termination_flag.store(true);
        _graph->_task_available_condition.notify_all();
    }

    _done.set_exception(std::current_exception());
}

void at::ThreadSeasonalWorker::process_tasks()
try
{
    _state.store(WorkerState::Delay);
    await_start_signal();

    at::IRunnable* frontElm = nullptr;
    do
    {
        {
            _state.store(WorkerState::Ready);
            std::unique_lock<std::mutex> lk{_pool->_task_queue_mutex};
            _pool->_work_available_condition.wait_for(
                lk, _alive_duration, [&]() { return _pool->_termination_flag.load() || !_pool->_task_queue.empty(); });
            _state.store(WorkerState::Busy);

            if (_pool->_termination_flag.load() || _pool->_task_queue.empty()) break;

            frontElm = _pool->_task_queue.front();
            _pool->_task_queue.pop();
            lk.unlock();
        };
        if (frontElm)
        {
            frontElm->_state.store(IRunnable::Executing);
            frontElm->execute();
            frontElm->_state.store(IRunnable::Completed);
            delete frontElm;
        }
        frontElm = nullptr;

    } while (true);

    AT_LOG("s-worker " << this->_id << " is exited");
    _state.store(WorkerState::Completed);
    _done.set_value();
}
catch (...)
{
    _done.set_exception(std::current_exception());
}

void at::ThreadPoolWorker::process_tasks()
try
{
    _state.store(WorkerState::Delay);
    await_start_signal();

    at::IRunnable* frontElm = nullptr;
    do
    {
        {
            _state.store(WorkerState::Ready);
            std::unique_lock<std::mutex> lk{_pool->_task_queue_mutex};
            _pool->_work_available_condition.wait(
                lk, [&]() { return (_pool->_termination_flag.load() || !_pool->_task_queue.empty()); });

            _state.store(WorkerState::Busy);

            if (_pool->_termination_flag.load())
            {
                lk.unlock();
                break;
            }
            frontElm = _pool->_task_queue.front();
            _pool->_task_queue.pop();
            lk.unlock();
        }

        if (frontElm)
        {
            frontElm->_state.store(IRunnable::Executing);
            frontElm->execute();
            frontElm->_state.store(IRunnable::Completed);
            delete frontElm;
        }
        _state.store(WorkerState::Busy);

        frontElm = nullptr;

    } while (true);

    AT_LOG("worker " << this->_id << " is exited");
    _state.store(WorkerState::Completed);
    _done.set_value();
}
catch (...)
{
    _done.set_exception(std::current_exception());
}
