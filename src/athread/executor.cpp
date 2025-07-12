#include "executor.h"

#include "threadgraph.h"

std::future<void> at::Executor::start(at::ThreadGraph& graph)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    std::thread(
        [&graph, promise = std::move(promise)]() mutable
        {
            try
            {
                graph.start();
                graph.wait();         // Ensure all previous tasks are completed before starting new ones.
                promise.set_value();  // Signal that the graph has started successfully.
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());  // Propagate any exceptions.
            }
        })
        .detach();  // Detach the thread to run independently.
    return future;  // Return the future to allow waiting for completion.
}

std::future<void> at::Executor::start_loop(at::ThreadGraph& graph, std::size_t times)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    std::thread(
        [&graph, times, promise = std::move(promise)]() mutable
        {
            try
            {
                for (std::size_t i = 0; i < times; ++i)
                {
                    graph.wait();  // Ensure all previous tasks are completed before starting new ones.
                    graph.start();
                }
                graph.wait();         // Wait for the last execution to complete.
                promise.set_value();  // Signal that the graph has started successfully.
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());  // Propagate any exceptions.
            }
        })
        .detach();  // Detach the thread to run independently.
    return future;  // Return the future to allow waiting for completion.
}
