#include <gtest/gtest.h>
#include "athread/athread.h"
#include <thread>
#include <chrono>

using namespace at;

class DummyNode : public INode {
public:
    DummyNode(std::chrono::milliseconds sleep_time) : sleep_time_(sleep_time) {}
    void execute() override {
        std::this_thread::sleep_for(sleep_time_);
    }
private:
    std::chrono::milliseconds sleep_time_;
};

TEST(ThreadGraphTest, WaitForReady) {
    ThreadGraph graph(1, false);
    graph.push(new DummyNode(std::chrono::milliseconds(100)));
    graph.start();
    auto status = graph.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(status, WaitStatus::Ready);
    graph.wait();  // Ensure the graph completes

}

TEST(ThreadGraphTest, WaitForTimeout) {
    ThreadGraph graph(1, false);
    graph.push(new DummyNode(std::chrono::seconds(1)));
    graph.start();
    auto status2 = graph.wait_for(std::chrono::milliseconds(100));
    EXPECT_EQ(status2, WaitStatus::Timeout);
    graph.wait();  // Ensure the graph completes
}