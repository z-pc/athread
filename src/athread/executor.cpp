#include "executor.h"

#include <limits>
#include <thread>

#include "threadgraph.h"

std::future<void> at::Executor::start(at::ThreadGraph& graph, std::size_t max_times,
                                      const std::function<bool()>& stop_condition,
                                      const std::function<void()>& callback)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    std::thread(
        [&graph, max_times, stop_condition, callback, promise = std::move(promise)]() mutable
        {
            try
            {
                std::size_t iteration = 0;
                bool stop_by_condition = false;

                while (iteration < max_times)
                {
                    graph.start();
                    graph.wait();
                    ++iteration;

                    if (stop_condition())
                    {
                        stop_by_condition = true;
                        break;
                    }
                }

                const bool stop_by_max_times = (iteration >= max_times);
                if (stop_by_condition || stop_by_max_times)
                {
                    callback();
                }

                promise.set_value();
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        })
        .detach();

    return future;
}

std::future<void> at::Executor::start(at::ThreadGraph& graph, const std::function<void()>& callback)
{
    return start(graph, 1, []() { return false; }, callback);
}

std::future<void> at::Executor::start_until(at::ThreadGraph& graph, const std::function<bool()>& stop_condition)
{
    return start_until(graph, stop_condition, []() {});
}

std::future<void> at::Executor::start(at::ThreadGraph& graph)
{
    return start(graph, 1, []() { return false; }, []() {});
}

std::future<void> at::Executor::start(at::ThreadGraph& graph, std::size_t times)
{
    return start(graph, times, []() { return false; }, []() {});
}

std::future<void> at::Executor::start_until(at::ThreadGraph& graph, std::size_t max_times,
                                            const std::function<bool()>& stop_condition)
{
    return start(graph, max_times, stop_condition, []() {});
}

std::future<void> at::Executor::start(at::ThreadGraph& graph, std::size_t times, const std::function<void()>& callback)
{
    return start(graph, times, []() { return false; }, callback);
}

std::future<void> at::Executor::start_until(at::ThreadGraph& graph, const std::function<bool()>& stop_condition,
                                            const std::function<void()>& callback)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    std::thread(
        [&graph, stop_condition, callback, promise = std::move(promise)]() mutable
        {
            try
            {
                while (true)
                {
                    graph.start();
                    graph.wait();

                    if (stop_condition())
                    {
                        break;
                    }
                }

                callback();
                promise.set_value();
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        })
        .detach();

    return future;
}
