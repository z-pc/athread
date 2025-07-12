#include <gtest/gtest.h>

#include <numeric>

#include "athread/athread.h"

using namespace at;
using namespace std;

TEST(ThreadGraph, CircularDependency)
{
    /*
     * Sample:
     *
    [1-R] --> [2-R]
      ^         |
      |---------|
     */

    ThreadGraph graph;
    auto t1 = graph.push([]() { AT_COUT("doing task1" << endl;); });
    auto t2 = graph.push([]() { AT_COUT("doing task2" << endl;); });

    // Create circular dependency
    t1.depend(t2);  // Node1 depends on Node2
    // Expect an exception or error to be thrown when starting the graph
    EXPECT_THROW(t2.depend(t1), std::runtime_error);
}

TEST(ThreadGraph, EraseTaskWhileExecuting)
{
    ThreadGraph graph;
    auto t1 = graph.push(
        []()
        {
            AT_COUT("doing task1" << endl;);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });

    auto t2 = graph.push(
        []()
        {
            AT_COUT("doing task2" << endl;);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });

    graph.start();
    EXPECT_THROW(graph.erase(t1), std::runtime_error);
    graph.wait();
}

TEST(ThreadGraph, EraseTaskSuccessfully)
{
    ThreadGraph graph;

    // Create tasks
    auto t1 = graph.push([]() { AT_COUT("Task 1" << endl); });
    auto t2 = graph.push([]() { AT_COUT("Task 2" << endl); });

    // Set up t2 to depend on t1
    t2.depend(t1);

    // Erase task t2 (which has no successors)
    EXPECT_TRUE(graph.erase(t2));

    graph.start();
    graph.wait();
}

TEST(ThreadGraph, EraseInvalidTask)
{
    ThreadGraph graph;

    // Create an invalid task (null)
    Task invalidTask;

    // Try to erase the invalid task
    EXPECT_FALSE(graph.erase(invalidTask));
}

TEST(ThreadGraph, EraseTaskNotInGraph)
{
    ThreadGraph graph;
    ThreadGraph anotherGraph;

    // Create a task in anotherGraph
    auto t1 = anotherGraph.push([]() { AT_COUT("Task 1" << endl); });

    // Try to erase a task that doesn't belong to graph
    EXPECT_FALSE(graph.erase(t1));
}

TEST(ThreadGraph, EraseTaskUpdatesDependencies)
{
    ThreadGraph graph;

    // Create tasks
    auto t1 = graph.push([]() { AT_COUT("Task 1" << endl); });
    auto t2 = graph.push([]() { AT_COUT("Task 2" << endl); });
    auto t3 = graph.push([]() { AT_COUT("Task 3" << endl); });

    // Set up dependencies: t1 -> t2 -> t3
    t2.depend(t1);
    t3.depend(t2);

    // Erase tasks
    // We need to erase t3 first since it depends on t2
    EXPECT_TRUE(graph.erase(t3));
    EXPECT_TRUE(graph.erase(t2));

    EXPECT_TRUE(t1.successors_size() == 0);

    // Run the graph to ensure it still works
    graph.start();
    graph.wait();
}

TEST(ThreadGraph, EraseMultipleTasksWithDependencies)
{
    ThreadGraph graph;

    // Create a chain of dependent tasks
    auto t1 = graph.push([]() { AT_COUT("Task 1" << endl); });
    auto t2 = graph.push([]() { AT_COUT("Task 2" << endl); });
    auto t3 = graph.push([]() { AT_COUT("Task 3" << endl); });
    auto t4 = graph.push([]() { AT_COUT("Task 4" << endl); });

    // Set up dependencies
    t2.depend(t1);
    t3.depend(t2);
    t4.depend(t3);

    // Erase tasks from back to front
    EXPECT_TRUE(graph.erase(t4));  // No successors - OK
    EXPECT_TRUE(graph.erase(t3));  // No successors after t4 is erased - OK
    EXPECT_TRUE(graph.erase(t2));  // No successors after t3 is erased - OK
    EXPECT_TRUE(graph.erase(t1));  // No successors after t2 is erased - OK

    // Verify graph is empty
    EXPECT_TRUE(graph.empty());
}

TEST(ThreadGraph, EraseTaskWithMultiplePredecessors)
{
    ThreadGraph graph;

    // Create tasks
    auto t1 = graph.push([]() { AT_COUT("Task 1" << endl); });
    auto t2 = graph.push([]() { AT_COUT("Task 2" << endl); });
    auto t3 = graph.push([]() { AT_COUT("Task 3" << endl); });

    // t3 depends on both t1 and t2
    t3.depend(t1);
    t3.depend(t2);

    // Erase t3
    EXPECT_TRUE(graph.erase(t3));

    // Verify t1 and t2 have no successors
    EXPECT_TRUE(t1.successors_size() == 0);
    EXPECT_TRUE(t2.successors_size() == 0);
}
//
// TEST(ThreadGraph, ClearTaskWhileExecuting)
//{
//    ThreadGraph graph;
//    auto t1 = graph.push(
//        []()
//        {
//            AT_COUT("doing task1" << endl;);
//            std::this_thread::sleep_for(std::chrono::microseconds(100));
//        });
//
//    graph.start();
//    EXPECT_THROW(graph.clear(), std::runtime_error);
//    graph.wait();
//}

TEST(ThreadGraph, PushTaskWhileExecuting)
{
    ThreadGraph graph;
    auto t1 = graph.push(
        []()
        {
            AT_COUT("doing task1" << endl;);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        });

    graph.start();

    EXPECT_THROW(graph.push(
                     []()
                     {
                         AT_COUT("doing task2" << endl;);
                         std::this_thread::sleep_for(std::chrono::microseconds(100));
                     }),
                 std::runtime_error);
    graph.wait();
}

TEST(ThreadGraph, LargeNumberOfTasks)
{
    ThreadGraph graph;

    const int numTasks = 1000;
    std::vector<Task> tasks;

    // Push a large number of tasks
    for (int i = 0; i < numTasks; ++i)
    {
        tasks.push_back(graph.push([i]() { AT_COUT("Task " << i << " executing" << endl;); }));
    }

    // Add dependencies between tasks
    for (int i = 1; i < numTasks; ++i)
    {
        tasks[i].depend(tasks[i - 1]);
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    SUCCEED();  // If no exceptions or errors occur, the test passes
}

TEST(ThreadGraph, InvalidTaskDependency)
{
    ThreadGraph graph;

    auto t1 = graph.push([]() { AT_COUT("Task 1 executing" << endl;); });

    Task invalidTask;  // Default constructed, invalid task

    // Attempt to add a dependency on an invalid task
    EXPECT_THROW(t1.depend(invalidTask), std::invalid_argument);
}

TEST(ThreadGraph, TaskStateAfterExecution)
{
    ThreadGraph graph;

    auto t1 = graph.push([]() { AT_COUT("Task 1 executing" << endl;); });

    graph.start();
    graph.wait();

    // Check if the task is in a valid state after execution
    EXPECT_TRUE(t1.state() == Task::COMPLETED);
}

TEST(ThreadGraph, TerminateBeforeCompletion)
{
    ThreadGraph graph;

    auto t1 = graph.push(
        []()
        {
            AT_COUT("Task 1 executing" << endl;);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });

    graph.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Terminate the graph before the task completes
    graph.terminate(false);
    graph.wait();
}

TEST(ThreadGraph, MultipleStartCalls)
{
    ThreadGraph graph;

    auto t1 = graph.push([]() { AT_COUT("Task 1 executing" << endl;); });

    graph.start();

    // Attempt to start the graph again while it is already running
    EXPECT_THROW(graph.start(), std::runtime_error);

    graph.wait();
}

TEST(ThreadGraph, PushCallableWithParameters)
{
    ThreadGraph graph;

    // Define a callable that accepts parameters
    auto callableWithParams = [](int a, int b)
    {
        AT_COUT("Task with parameters: a = " << a << ", b = " << b << endl;);
        EXPECT_EQ(a + b, 5);  // Verify the sum of parameters
    };

    // Push the callable with parameters into the graph
    auto t1 = graph.push(callableWithParams, 2, 3);

    // Start the graph and wait for completion
    graph.start();
    graph.wait();
}

TEST(ThreadGraph, ParallelComputationAndResultAggregation)
{
    ThreadGraph graph;

    std::atomic<int> result{0};  // Variable to aggregate results

    // Push tasks for parallel computation
    auto t1 = graph.push([&result]() { result += 10; });
    auto t2 = graph.push([&result]() { result += 20; });
    auto t3 = graph.push([&result]() { result += 30; });

    // Ensure tasks are executed in sequence
    t2.depend(t1);  // t2 depends on t1
    t3.depend(t2);  // t3 depends on t2

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    // Validate the result
    EXPECT_EQ(result.load(), 60);  // Sum 10 + 20 + 30 = 60
}

TEST(ThreadGraph, FibonacciParallelComputation)
{
    ThreadGraph graph;

    std::vector<int> fib(10, 0);  // Fibonacci sequence
    fib[0] = 0;
    fib[1] = 1;

    // Push tasks to compute Fibonacci numbers
    for (int i = 2; i < 10; ++i)
    {
        graph.push(
            [i, &fib]()
            {
                fib[i] = fib[i - 1] + fib[i - 2];
                AT_COUT("Fib[" << i << "] = " << fib[i] << endl;);
            });
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    // Validate the Fibonacci sequence
    EXPECT_EQ(fib[2], 1);
    EXPECT_EQ(fib[3], 2);
    EXPECT_EQ(fib[4], 3);
    EXPECT_EQ(fib[5], 5);
    EXPECT_EQ(fib[6], 8);
    EXPECT_EQ(fib[7], 13);
    EXPECT_EQ(fib[8], 21);
    EXPECT_EQ(fib[9], 34);
}

TEST(ThreadGraph, MatrixSumParallel)
{
    ThreadGraph graph;

    std::vector<std::vector<int>> matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::atomic<int> totalSum{0};

    // Push tasks to compute the sum of each row
    for (const auto& row : matrix)
    {
        graph.push(
            [&row, &totalSum]()
            {
                int rowSum = std::accumulate(row.begin(), row.end(), 0);
                totalSum += rowSum;
            });
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    // Validate the total sum
    EXPECT_EQ(totalSum.load(), 45);  // Sum of all elements 1+2+3+4+5+6+7+8+9 = 45
}

TEST(ThreadGraph, ParallelPrefixSum)
{
    ThreadGraph graph;

    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> prefixSum(input.size(), 0);

    // Push tasks to compute prefix sum
    for (size_t i = 0; i < input.size(); ++i)
    {
        graph.push(
            [i, &input, &prefixSum]()
            {
                prefixSum[i] = (i == 0) ? input[i] : prefixSum[i - 1] + input[i];
                AT_COUT("PrefixSum[" << i << "] = " << prefixSum[i] << endl;);
            });
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    // Validate the prefix sum
    EXPECT_EQ(prefixSum[0], 1);
    EXPECT_EQ(prefixSum[1], 3);
    EXPECT_EQ(prefixSum[2], 6);
    EXPECT_EQ(prefixSum[3], 10);
    EXPECT_EQ(prefixSum[4], 15);
}

TEST(ThreadGraph, MoveConstructor)
{
    ThreadGraph graph1;
    auto t1 = graph1.push([]() { AT_COUT("Task 1" << std::endl); });
    auto t2 = graph1.push([]() { AT_COUT("Task 2" << std::endl); });
    t2.depend(t1);

    // Move construct graph2 from graph1
    ThreadGraph graph2(std::move(graph1));

    // graph1 should now be empty (moved-from)
    EXPECT_TRUE(graph1.empty());

    // graph2 should have the tasks
    EXPECT_EQ(graph2.task_size(), 2);

    // The tasks in graph2 should be executable
    graph2.start();
    graph2.wait();

    // Optionally, check that tasks are completed
    EXPECT_TRUE(graph2.task_at(0).state() == Task::COMPLETED || graph2.task_at(1).state() == Task::COMPLETED);
}

TEST(Executor, StartGraphAsync)
{
    ThreadGraph graph;
    std::atomic<int> result{0};

    auto t1 = graph.push([&result]() { result += 1; });
    auto t2 = graph.push([&result]() { result += 2; });
    t2.depend(t1);

    at::Executor executor;
    auto fut = executor.start(graph);

    // Wait for async execution to finish
    fut.get();

    EXPECT_EQ(result.load(), 3);
    EXPECT_TRUE(t1.state() == Task::COMPLETED);
    EXPECT_TRUE(t2.state() == Task::COMPLETED);
}

TEST(Executor, StartLoopGraphAsync)
{
    ThreadGraph graph;
    std::atomic<int> result{0};

    auto t1 = graph.push([&result]() { result += 1; });
    auto t2 = graph.push([&result]() { result += 2; });
    t2.depend(t1);

    at::Executor executor;
    auto fut = executor.start_loop(graph, 3);

    // Wait for async execution to finish
    fut.get();

    // Each loop: result += 1 + 2 = 3, loop 3 times: 3 * 3 = 9
    EXPECT_EQ(result.load(), 9);
    EXPECT_TRUE(t1.state() == Task::COMPLETED);
    EXPECT_TRUE(t2.state() == Task::COMPLETED);
}

TEST(Executor, ExceptionInTaskPropagatesToFuture)
{
    ThreadGraph graph;
    auto t1 = graph.push([]() { throw std::runtime_error("Task error"); });

    at::Executor executor;
    auto fut = executor.start(graph);

    // The future should throw when get() is called
    EXPECT_THROW(fut.get(), std::runtime_error);
}
