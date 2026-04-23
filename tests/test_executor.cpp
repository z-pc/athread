#include <gtest/gtest.h>

#include <atomic>

#include "athread/athread.h"

using namespace at;

class CountingNode : public INode
{
public:
    explicit CountingNode(std::atomic<int>& counter) : _counter(counter) {}

    void execute() override { ++_counter; }

private:
    std::atomic<int>& _counter;
};

TEST(ExecutorTest, Start_MaxTimesStopConditionCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};
    std::atomic<int> callback_count{0};

    graph.push(new CountingNode(runs));

    auto future =
        executor.start(graph, 10, [&runs]() { return runs.load() >= 3; }, [&callback_count]() { ++callback_count; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 3);
    EXPECT_EQ(callback_count.load(), 1);
}

TEST(ExecutorTest, Start_WithCallback_RunsOnce)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};
    std::atomic<int> callback_count{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(graph, [&callback_count]() { ++callback_count; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 1);
    EXPECT_EQ(callback_count.load(), 1);
}

TEST(ExecutorTest, Start_WithStopCondition_NoCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start_until(graph, [&runs]() -> bool { return runs.load() >= 2; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 2);
}

TEST(ExecutorTest, Start_NoArgs_RunsOnce)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(graph);

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 1);
}

TEST(ExecutorTest, Start_WithTimes_RunsExactCount)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(graph, static_cast<std::size_t>(4));

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 4);
}

TEST(ExecutorTest, Start_MaxTimesStopCondition_NoCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(graph, static_cast<std::size_t>(3), []() { return false; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 3);
}

TEST(ExecutorTest, Start_TimesWithCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};
    std::atomic<int> callback_count{0};

    graph.push(new CountingNode(runs));

    auto future = executor.start(graph, static_cast<std::size_t>(5), [&callback_count]() { ++callback_count; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 5);
    EXPECT_EQ(callback_count.load(), 1);
}

TEST(ExecutorTest, Start_StopConditionWithCallback)
{
    ThreadGraph graph(1, false);
    Executor executor;
    std::atomic<int> runs{0};
    std::atomic<int> callback_count{0};

    graph.push(new CountingNode(runs));

    auto future =
        executor.start_until(graph, [&runs]() { return runs.load() >= 4; }, [&callback_count]() { ++callback_count; });

    ASSERT_NO_THROW(future.get());
    EXPECT_EQ(runs.load(), 4);
    EXPECT_EQ(callback_count.load(), 1);
}