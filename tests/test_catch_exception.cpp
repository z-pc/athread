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

        //graph.wait();
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
    ExceptionNode* nodeA = new ExceptionNode(); // Will throw
    FlagNode* nodeB = new FlagNode(b_executed); // Should NOT execute
    FlagNode* nodeC = new FlagNode(c_executed); // Should NOT execute

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