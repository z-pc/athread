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
                bool condition_met = false;
                bool terminated_by_request = false;

                while (iteration < max_times)
                {
                    graph.start();
                    graph.wait();
                    ++iteration;

                    if (graph.stop_reason() == StopReason::StoppedByRequest)
                    {
                        terminated_by_request = true;
                        break;
                    }

                    if (stop_condition())
                    {
                        condition_met = true;
                        graph.stop_reason(StopReason::StoppedByRequest);
                        break;
                    }
                }

                const bool max_times_reached = (!terminated_by_request && !condition_met && iteration >= max_times);

                if (max_times_reached)
                {
                    graph.stop_reason(StopReason::LimitReached);
                }
                if (condition_met || max_times_reached)
                {
                    callback();
                }

                promise.set_value();
            }
            catch (...)
            {
                graph.stop_reason(StopReason::Error);
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
                bool condition_met = false;

                while (true)
                {
                    graph.start();
                    graph.wait();

                    if (graph.stop_reason() == StopReason::StoppedByRequest)
                    {
                        break;
                    }

                    if (stop_condition())
                    {
                        condition_met = true;
                        graph.stop_reason(StopReason::StoppedByRequest);
                        break;
                    }
                }

                if (condition_met)
                {
                    callback();
                }

                promise.set_value();
            }
            catch (...)
            {
                graph.stop_reason(StopReason::Error);
                promise.set_exception(std::current_exception());
            }
        })
        .detach();

    return future;
}
