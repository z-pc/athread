#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "athread/athread.h"

using namespace at;

// Dummy node that throws in execute
class ExceptionNode : public INode
{
public:
    void execute() override { throw std::runtime_error("Test exception from worker"); }
};

// Dummy node that sets a flag when executed
class FlagNode : public INode
{
public:
    FlagNode(bool& flag) : _flag(flag) {}
    void execute() override { _flag = true; }

private:
    bool& _flag;
};

class CountingNode : public INode
{
public:
    explicit CountingNode(std::atomic<int>& counter) : _counter(counter) {}

    void execute() override { ++_counter; }

private:
    std::atomic<int>& _counter;
};

class SignalSleepNode : public INode
{
public:
    SignalSleepNode(std::atomic<bool>& started, std::chrono::milliseconds sleep_time)
        : _started(started), _sleep_time(sleep_time)
    {
    }

    void execute() override
    {
        _started.store(true);
        std::this_thread::sleep_for(_sleep_time);
    }

private:
    std::atomic<bool>& _started;
    std::chrono::milliseconds _sleep_time;
};

class ThrowNode : public INode
{
public:
    void execute() override { throw std::runtime_error("Executor test exception"); }
};

TEST(ThreadGraphTest, WorkerPromiseExceptionIsCaughtInWait)
{
    ThreadGraph graph(2, false);
    graph.push(new ExceptionNode());
    graph.start();

    // wait() should throw and the message should contain our exception
    try
    {
        graph.wait();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Test exception from worker"), std::string::npos)
            << "Exception message should contain the thrown text";
    }
    catch (...)
    {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(ThreadGraphTest, WorkerPromiseExceptionIsCaughtInWaitFor)
{
    ThreadGraph graph(2, false);
    graph.push(new ExceptionNode());
    graph.start();

    try
    {
        // wait_for should return READY, and then wait() will throw
        auto status = graph.wait_for(std::chrono::seconds(2));
        // EXPECT_EQ(status, WaitStatus::READY);

        // graph.wait();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Test exception from worker"), std::string::npos)
            << "Exception message should contain the thrown text";
    }
    catch (...)
    {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(ThreadGraphTest, GraphStopsAndSkipsAllDependentsUsingTaskDepend)
{
    bool b_executed = false;
    bool c_executed = false;

    ThreadGraph graph(2, false);

    // Create nodes
    ExceptionNode* nodeA = new ExceptionNode();  // Will throw
    FlagNode* nodeB = new FlagNode(b_executed);  // Should NOT execute
    FlagNode* nodeC = new FlagNode(c_executed);  // Should NOT execute

    // Wrap nodes in Task handles
    Task taskA = graph.push(nodeA);
    Task taskB = graph.push(nodeB);
    Task taskC = graph.push(nodeC);

    // Set up dependencies: B depends on A, C depends on B
    taskB.depend(taskA);
    taskC.depend(taskB);

    graph.start();

    try
    {
        graph.wait();
        FAIL() << "Expected std::runtime_error";
    }
    catch (const std::runtime_error& e)
    {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Test exception from worker"), std::string::npos)
            << "Exception message should contain the thrown text";
    }
    catch (...)
    {
        FAIL() << "Expected std::runtime_error";
    }

    // Dependent nodes should NOT have executed
    EXPECT_FALSE(b_executed) << "Dependent node B should not execute after exception";
    EXPECT_FALSE(c_executed) << "Dependent node C should not execute after exception";
}

TEST(ExecutorTest, Start_MaxTimesReached_InvokesCallbackAndSetsLimitReached)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};
    std::atomic<int> callback_count{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(
        graph, static_cast<std::size_t>(3), []() { return false; }, [&callback_count]() { ++callback_count; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 3);
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(graph.stop_reason(), StopReason::LimitReached);
}

TEST(ExecutorTest, StartUntil_TerminatedByRequest_DoesNotInvokeCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<bool> started{false};
    std::atomic<int> callback_count{0};

    graph.push(new SignalSleepNode(started, std::chrono::milliseconds(200)));

    auto future = executor.start_until(graph, []() { return false; }, [&callback_count]() { ++callback_count; });

    for (int i = 0; i < 100 && !started.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_TRUE(started.load());

    graph.terminate(false);

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(callback_count.load(), 0);
    EXPECT_EQ(graph.stop_reason(), StopReason::StoppedByRequest);
}

TEST(ExecutorTest, StartUntil_Error_DoesNotInvokeCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> callback_count{0};

    graph.push(new ThrowNode());

    auto future = executor.start_until(graph, []() { return false; }, [&callback_count]() { ++callback_count; });

    EXPECT_THROW(future.get(), std::runtime_error);
    EXPECT_EQ(callback_count.load(), 0);
    EXPECT_EQ(graph.stop_reason(), StopReason::Error);
}