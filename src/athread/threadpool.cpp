//////////////////////////////////////////////////////////////////////////
//
// Copyright 2022 Le Xuan Tuan Anh
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

#include "threadpool.h"

#include <memory>
#include <thread>

#include "worker.h"

using namespace at;
using namespace std;

at::ThreadPool::ThreadPool(std::uint32_t core_thread_count, std::uint32_t max_thread_count,
                           const std::chrono::nanoseconds& alive_seasonal_time, bool wait_for_start_signal)
{
    _core_thread_count = core_thread_count;
    _max_thread_count = max_thread_count;
    _alive_seasonal_time = alive_seasonal_time;
    _wait_for_start_signal = wait_for_start_signal;
    _termination_flag = false;
}

at::ThreadPool::~ThreadPool()
{
    terminate();
    clear();
}

bool at::ThreadPool::push(IRunnable* runnable)
{
    if (!executable()) return false;

    clean_complete_workers();

    if (_worker_contexts.size() < _max_thread_count || _max_thread_count == 0)
    {
        bool create = true;
        for (auto& context : _worker_contexts)
        {
            if (context && context->worker->state() == at::WorkerState::Ready)
            {
                create = false;
                break;
            }
        }

        if (create)
        {
            // Check if the number for main work is full, so we need to create seasonal workers.
            if (_worker_contexts.size() >= _core_thread_count)
                create_seasonal_worker(1, _alive_seasonal_time);
            else
                create_worker(1);
        }
    }

    {
        std::lock_guard<std::mutex> lk{_task_queue_mutex};
        _task_queue.push(runnable);
        _work_available_condition.notify_one();
    }

    return true;
}

void ThreadPool::clear()
{
    std::lock_guard<std::mutex> lock(_task_queue_mutex);
    while (!_task_queue.empty())
    {
        delete _task_queue.front();
        _task_queue.pop();
    }
}

void at::ThreadPool::start()
{
    _wait_for_start_signal.store(false);
    _termination_flag.store(false);
    _work_available_condition.notify_all();
}

void at::ThreadPool::wait()
{
    clean_complete_workers();

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

void at::ThreadPool::terminate(bool alsoWait)
{
    _termination_flag.store(true);
    _work_available_condition.notify_all();
    if (alsoWait) wait();
}

bool ThreadPool::empty()
{
    std::lock_guard<std::mutex> lock(_task_queue_mutex);
    return _task_queue.empty();
}

bool ThreadPool::executable() const { return !_termination_flag.load(); }

std::uint32_t ThreadPool::generate_worker_uid() const { return _worker_contexts.size(); }

void at::ThreadPool::create_worker(std::uint32_t count)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        std::unique_ptr<WorkerContext> context = std::make_unique<WorkerContext>();
        context->worker.reset(new ThreadPoolWorker(generate_worker_uid(), this));
        context->future = context->worker->get_future();
        context->thread = std::thread(&IWorker::process_tasks, context->worker.get());
        _worker_contexts.push_back(std::move(context));
    }
}

void ThreadPool::create_seasonal_worker(std::uint32_t count, const std::chrono::nanoseconds& alive_duration)
{
    for (std::uint32_t i = 0; i < count; i++)
    {
        std::unique_ptr<WorkerContext> context = std::make_unique<WorkerContext>();
        context->worker.reset(new ThreadSeasonalWorker(generate_worker_uid(), this, alive_duration));
        context->future = context->worker->get_future();
        context->thread = std::thread(&IWorker::process_tasks, context->worker.get());
        _worker_contexts.push_back(std::move(context));
    }
}

void at::ThreadPool::clean_complete_workers()
{
    auto contextIt = _worker_contexts.begin();

    while (contextIt != _worker_contexts.end())
    {
        auto& context = *contextIt;

        if (context->worker->state() == at::WorkerState::Completed)
        {
            if (context->thread.joinable())
            {
                context->thread.join();  // validate again to make sure a worker is really ended;
            }

            contextIt = _worker_contexts.erase(contextIt);  // Remove completed worker context
        }
        else
        {
            ++contextIt;  // Move to the next worker context
        }
    }
}

void ThreadPool::reset()
{
    _termination_flag.store(false);
    _wait_for_start_signal.store(true);
    clean_complete_workers();
    _worker_contexts.clear();
}

ThreadPoolFixed::ThreadPoolFixed(std::uint32_t coreSize) : ThreadPool(coreSize, coreSize, 0s, true) {}

ThreadPoolFixed::~ThreadPoolFixed() {}

void ThreadPoolFixed::create_worker(std::uint32_t count) { create_seasonal_worker(count, 0s); }

bool ThreadPoolFixed::executable() const
{
    if (_termination_flag.load() == true) return false;
    if (_wait_for_start_signal.load() == true) return true;
    return _worker_contexts.size() > 0;
}
