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

TEST(ThreadGraphTest, WorkerPromiseExceptionIsCaughtInWait)
{
    ThreadGraph graph(1, false);
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
    ThreadGraph graph(1, false);
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