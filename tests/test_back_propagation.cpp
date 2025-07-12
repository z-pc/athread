/*
* The template graph for testing. We retiveve nodes information from a random node.
* Node format: [OrderNode-StateSymbol]
* Node state symbols:
* C: IRunnable::COMPLETE
* E: IRunnable::EXECUTING
* R: IRunnable::READY
*
[1-C]   [2-E]     [3-E]    [ 4-R ]
    \    /           \    /
   [ 5-R ]  [ 6-R ] [ 7-R ]  [ 8-C ]
       \     /  \      |    /
        \   /    \     |   /  [ 11-R ]
         \ /      \    |  / /
       [ 9-R ]     [ 10-R ]
            \       /
            [ 12-R ]

*/

#include "athread/athread.h"
#include <gtest/gtest.h>
#include <algorithm>

using namespace athread;
using namespace std;

class MyNode : public INode
{
public:
    MyNode(const string& nodeName, int state) : _name{nodeName} { setStateViaProxy(state); }
    void execute() override { ATHREAD_COUT("Node name: " << _name); }
    std::string _name;
    void setStateViaProxy(int state)
    {
        struct Proxy : MyNode
        {
            using MyNode::setState; // Expose setState
        };
        static_cast<Proxy*>(this)->setState(state);
    }
};

class MyGraph : public ThreadGraph, public ::testing::Test
{
public:
    std::vector<Task> tasks;
    MyGraph(int numCore) : ThreadGraph(numCore) {}
    MyGraph() : ThreadGraph(1) {}

    virtual std::pair<NodeExecutionState, INode*> getNextNode(const INode* entryNode) override
    {
        return __super::getNextNode(entryNode);
    }

    // overide push function

    virtual Task push(athread::INode* Node) override
    {
        auto state = Node->getState();
        ((MyNode*)Node)->setStateViaProxy(INode::READY);
        auto task = __super::push(Node);
        ((MyNode*)Node)->setStateViaProxy(state);
        return task;
    }

    virtual void start() override
    {
        wait();
        _terminalSignal.store(false); // Reset the terminal signal to false before starting.

        // copy ready task from _tasks to _readyTasksPool
        std::copy_if(_tasks.begin(), _tasks.end(), std::back_inserter(_readyTasksPool),
                     [](INode* node) { return node->getState() == INode::READY; });
    }

    void SetUp() override
    {
        tasks = std::vector<Task>(12);
        tasks[0] = push(new MyNode("node1", IRunnable::COMPLETED));
        tasks[1] = push(new MyNode("node2", IRunnable::EXECUTING));
        tasks[2] = push(new MyNode("node3", IRunnable::EXECUTING));
        tasks[3] = push(new MyNode("node4", IRunnable::READY));
        tasks[4] = push(new MyNode("node5", IRunnable::READY));
        tasks[5] = push(new MyNode("node6", IRunnable::READY));
        tasks[6] = push(new MyNode("node7", IRunnable::READY));
        tasks[7] = push(new MyNode("node8", IRunnable::COMPLETED));
        tasks[8] = push(new MyNode("node9", IRunnable::READY));
        tasks[9] = push(new MyNode("node10", IRunnable::READY));
        tasks[10] = push(new MyNode("node11", IRunnable::READY));
        tasks[11] = push(new MyNode("node12", IRunnable::READY));

        tasks[4].after({tasks[0], tasks[1]});
        tasks[6].after({tasks[2], tasks[3]});
        tasks[8].after({tasks[4], tasks[5]});
        tasks[9].after({tasks[5], tasks[6], tasks[7], tasks[10]});
        tasks[11].after({tasks[8], tasks[9]});
    }

    virtual std::pair<NodeExecutionState, INode*> backPropagationNode(
        const INode* entryNode,
        const std::unordered_set<const INode*> avoids = std::unordered_set<const INode*>()) const override
    {
        return __super::backPropagationNode(entryNode);
    }
};

TEST_F(MyGraph, BackPropagationNodeFunction_N2)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[1].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::WAITING);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node2");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N4)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[3].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node4");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N5)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[4].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::WAITING);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node2");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N6)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[5].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node6");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N7)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[6].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node4");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N8)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[7].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::COMPLETED);
    auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node8");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N9)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[8].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node6");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N11)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[10].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    EXPECT_EQ(static_cast<MyNode*>(retrievedNode.second)->_name, "node11");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N10)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[9].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node6" || nodeName == "node4");
    EXPECT_FALSE(nodeName == "node7");
}

TEST_F(MyGraph, BackPropagationNodeFunction_N12)
{
    auto retrievedNode = backPropagationNode((INode*)tasks[11].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node5" || nodeName == "node6" || nodeName == "node4" || nodeName == "node11");
    EXPECT_FALSE(nodeName == "node7" && nodeName == "node9" && nodeName == "node10" && nodeName == "node12");
}
