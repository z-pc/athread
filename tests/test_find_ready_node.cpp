
/*
 * The template graph for testing.
 * Node format: [OrderNode-StateSymbol]
 * Node state symbols:
 * C: IRunnable::COMPLETE
 * E: IRunnable::EXECUTING
 * R: IRunnable::READY
 */

#include "athread/athread.h"
#include <algorithm>
#include <gtest/gtest.h>

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

class MyGraph : public ThreadGraph
{
public:
    std::vector<Task> tasks;
    MyGraph(int numCore) : ThreadGraph(numCore) {}
    MyGraph() : ThreadGraph(1) {}

    virtual std::pair<NodeExecutionState, INode*> trace_ready_node(const INode* entryNode) override
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
};

TEST(MyGraph, GetNextNodeFunction_AllPrecedesNotComplete)
{
    /*
     * Sample:
     *
    [1-R]   [2-E]
        \    /
        [ 3-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(3);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::READY));     // Node1: READY
    tasks[1] = graph.push(new MyNode("node2", IRunnable::EXECUTING)); // Node2: EXECUTING
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY));     // Node3: READY
    tasks[2].after({tasks[0], tasks[1]});                             // Node3 depends on Node1 and Node2
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        if (t.getNode() == tasks[1].getNode()) // Tracking Node2 (EXECUTING)
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_EQ(nodeName, "node1"); // Node1 is READY and has no precedes
        }
    }
}

TEST(MyGraph, GetNextNodeFunction_AllPrecedesComplete)
{
    /*
     * Sample:
     *
    [1-C]   [2-C]
        \    /
        [ 3-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(3);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED)); // Node1: COMPLETE
    tasks[1] = graph.push(new MyNode("node2", IRunnable::COMPLETED)); // Node2: COMPLETE
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY));     // Node3: READY
    tasks[2].after({tasks[0], tasks[1]});                             // Node3 depends on Node1 and Node2
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        if (t.getNode() == tasks[2].getNode()) // Tracking Node3
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_EQ(nodeName, "node3"); // Node3 is READY and all precedes are COMPLETE
        }
    }
}

TEST(MyGraph, GetNextNodeFunction_PrecedesMixedStates)
{
    /*
     * Sample:
     *
    [1-C]   [2-E]
        \    /
        [ 3-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(3);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED)); // Node1: COMPLETE
    tasks[1] = graph.push(new MyNode("node2", IRunnable::EXECUTING)); // Node2: EXECUTING
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY));     // Node3: READY
    tasks[2].after({tasks[0], tasks[1]});                             // Node3 depends on Node1 and Node2
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::WAITING);
        EXPECT_EQ(retrievedNode.second, (INode*)tasks[1].getNode()); // Node3 is WAITING because Node2 is EXECUTING
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node2");
    }
}

TEST(MyGraph, GetNextNodeFunction_ReadyPrecedes)
{
    /*
     * Sample:
     *
    [1-R]   [2-R]
        \    /
        [ 3-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(3);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::READY)); // Node1: READY
    tasks[1] = graph.push(new MyNode("node2", IRunnable::READY)); // Node2: READY
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY)); // Node3: READY
    tasks[2].after({tasks[0], tasks[1]});                         // Node3 depends on Node1 and Node2
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        if (t.getNode() == tasks[0].getNode() || t.getNode() == tasks[1].getNode()) // Tracking Node1 or Node2
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
        }
        else if (t.getNode() == tasks[2].getNode()) // Tracking Node3
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr); // Node3 is WAITING because Node1 and Node2 are not COMPLETE
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_TRUE(nodeName == "node1" || nodeName == "node2");
        }
    }
}

TEST(MyGraph, GetNextNodeFunction_MultipleBranches)
{
    /*
     * Sample:
     *
    [1-C]   [2-C]  [4-C] [6-C]
        \    /        \   /
        [ 3-C ]       [5-R]
     */

    MyGraph graph;
    std::vector<Task> tasks(6);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED)); // Node1: COMPLETE
    tasks[1] = graph.push(new MyNode("node2", IRunnable::COMPLETED)); // Node2: COMPLETE
    tasks[2] = graph.push(new MyNode("node3", IRunnable::COMPLETED)); // Node3: COMPLETE
    tasks[3] = graph.push(new MyNode("node4", IRunnable::COMPLETED)); // Node4: COMPLETE
    tasks[4] = graph.push(new MyNode("node5", IRunnable::READY));     // Node5: READY
    tasks[5] = graph.push(new MyNode("node6", IRunnable::COMPLETED)); // Node6: COMPLETE
    tasks[2].after({tasks[0], tasks[1]});
    tasks[4].after({tasks[3], tasks[5]});
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        if (t.getNode() == tasks[4].getNode()) // Tracking Node5
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_EQ(nodeName, "node5"); // Node5 is READY and all precedes are COMPLETE
        }
    }
}

TEST(MyGraph, GetNextNodeFunction_MultipleDependenciesMixedStates)
{
    /*
     * Sample:
     *
    [1-C]   [2-E]   [3-R]
        \    |       |
         \   |       |
          [ 4-R ]   [5-R]
               \     /
                [ 6-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(6);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED)); // Node1: COMPLETE
    tasks[1] = graph.push(new MyNode("node2", IRunnable::EXECUTING)); // Node2: EXECUTING
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY));     // Node3: READY
    tasks[3] = graph.push(new MyNode("node4", IRunnable::READY));     // Node4: READY
    tasks[4] = graph.push(new MyNode("node5", IRunnable::READY));     // Node5: READY
    tasks[5] = graph.push(new MyNode("node6", IRunnable::READY));     // Node6: READY

    tasks[3].after({tasks[0], tasks[1]}); // Node4 depends on Node1 and Node2
    tasks[4].after({tasks[2]});           // Node5 depends on Node3
    tasks[5].after({tasks[3], tasks[4]}); // Node6 depends on Node4 and Node5
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        if (t.getNode() == tasks[1].getNode()) // Tracking Node2 (EXECUTING)
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_EQ(nodeName, "node3"); // Node3 is READY and has no precedes
        }
        else if (t.getNode() == tasks[3].getNode()) // Tracking Node4
        {
            EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
            EXPECT_NE(retrievedNode.second, nullptr);
            auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
            EXPECT_EQ(nodeName, "node3");
        }
    }
}

TEST(MyGraph, GetNextNodeFunction_CircularDependency)
{
    /*
     * Sample:
     *
    [1-R] --> [2-R]
      ^         |
      |---------|
     */

    MyGraph graph;
    std::vector<Task> tasks(2);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::READY)); // Node1: READY
    tasks[1] = graph.push(new MyNode("node2", IRunnable::READY)); // Node2: READY

    // Create circular dependency
    tasks[0].after({tasks[1]}); // Node1 depends on Node2
    // tasks[1].after({tasks[0]}); // Node2 depends on Node1

    // Expect an exception or error to be thrown when starting the graph
    EXPECT_THROW(tasks[1].after({tasks[0]}), std::runtime_error);
}

TEST(MyGraph, GetNextNodeFunction_ComplexGraphMultipleBranches)
{
    /*
     * Sample:
     *
    [1-C]   [2-C]   [3-R]
        \    /       |
         [ 4-R ]    [5-R]
          \   \     /
           \   [6-R]
            \  /
            [ 7-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(7);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED)); // Node1: COMPLETE
    tasks[1] = graph.push(new MyNode("node2", IRunnable::COMPLETED)); // Node2: COMPLETE
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY));     // Node3: READY
    tasks[3] = graph.push(new MyNode("node4", IRunnable::READY));     // Node4: READY
    tasks[4] = graph.push(new MyNode("node5", IRunnable::READY));     // Node5: READY
    tasks[5] = graph.push(new MyNode("node6", IRunnable::READY));     // Node6: READY
    tasks[6] = graph.push(new MyNode("node7", IRunnable::READY));     // Node7: READY

    tasks[3].after({tasks[0], tasks[1]}); // Node4 depends on Node1 and Node2
    tasks[4].after({tasks[2]});           // Node5 depends on Node3
    tasks[5].after({tasks[3], tasks[4]}); // Node6 depends on Node4 and Node5
    tasks[6].after({tasks[5], tasks[2]}); // Node7 depends on Node6 and Node3
    graph.start();

    // Tracking Node1
    {
        auto retrievedNode = graph.getNextNode(tasks[0].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node4"); // Node1 is COMPLETE, so Node4 is the next node
    }

    // Tracking Node2
    {
        auto retrievedNode = graph.getNextNode(tasks[1].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node4"); // Node2 is COMPLETE, so Node4 is the next node
    }

    // Tracking Node3
    {
        auto retrievedNode = graph.getNextNode(tasks[2].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node3"); // Node3 is READY, so Node3 is the next node
    }

    // Tracking Node4
    {
        auto retrievedNode = graph.getNextNode(tasks[3].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node4"); // Node4 is READY, so Node4 is the next node
    }

    // Tracking Node5
    {
        auto retrievedNode = graph.getNextNode(tasks[4].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_EQ(nodeName, "node3"); // Node5 pends on Node3, so Node3 is the next node
    }

    // Tracking Node6
    {
        auto retrievedNode = graph.getNextNode(tasks[5].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_TRUE(nodeName == "node4" || nodeName == "node3"); // Node6 depends on Node4 and Node3
    }

    // Tracking Node7
    {
        auto retrievedNode = graph.getNextNode(tasks[6].getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
        auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
        EXPECT_TRUE(nodeName == "node4" || nodeName == "node3"); // Node7 depends on Node4 and Node3
    }
}

TEST(MyGraph, GetNextNodeFunction_AllNodesReady)
{
    /*
     * Sample:
     *
    [1-R]   [2-R]   [3-R]
        \    /       |
         [ 4-R ]    [5-R]
          \   \     /
           \   [6-R]
            \  /
            [ 7-R ]
     */

    MyGraph graph;
    std::vector<Task> tasks(7);
    tasks[0] = graph.push(new MyNode("node1", IRunnable::READY)); // Node1: READY
    tasks[1] = graph.push(new MyNode("node2", IRunnable::READY)); // Node2: READY
    tasks[2] = graph.push(new MyNode("node3", IRunnable::READY)); // Node3: READY
    tasks[3] = graph.push(new MyNode("node4", IRunnable::READY)); // Node4: READY
    tasks[4] = graph.push(new MyNode("node5", IRunnable::READY)); // Node5: READY
    tasks[5] = graph.push(new MyNode("node6", IRunnable::READY)); // Node6: READY
    tasks[6] = graph.push(new MyNode("node7", IRunnable::READY)); // Node7: READY

    tasks[3].after({tasks[0], tasks[1]}); // Node4 depends on Node1 and Node2
    tasks[5].after({tasks[3], tasks[4]}); // Node6 depends on Node4 and Node5
    tasks[6].after({tasks[5], tasks[2]}); // Node7 depends on Node6 and Node3
    tasks[4].after({tasks[2]});           // Node5 depends on Node3
    graph.start();

    for (const auto& t : tasks)
    {
        auto retrievedNode = graph.getNextNode(t.getNode());
        EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
        EXPECT_NE(retrievedNode.second, nullptr);
    }
}

TEST(MyGraph, GetNextNodeFunction_jhfkd23d)
{
    /*
    * Sample:
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

    std::vector<Task> tasks(12);
    MyGraph graph;
    tasks[0] = graph.push(new MyNode("node1", IRunnable::COMPLETED));
    tasks[1] = graph.push(new MyNode("node2", IRunnable::EXECUTING));
    tasks[2] = graph.push(new MyNode("node3", IRunnable::EXECUTING));
    tasks[3] = graph.push(new MyNode("node4", IRunnable::READY));
    tasks[4] = graph.push(new MyNode("node5", IRunnable::READY));
    tasks[5] = graph.push(new MyNode("node6", IRunnable::READY));
    tasks[6] = graph.push(new MyNode("node7", IRunnable::READY));
    tasks[7] = graph.push(new MyNode("node8", IRunnable::COMPLETED));
    tasks[8] = graph.push(new MyNode("node9", IRunnable::READY));
    tasks[9] = graph.push(new MyNode("node10", IRunnable::READY));
    tasks[10] = graph.push(new MyNode("node11", IRunnable::READY));
    tasks[11] = graph.push(new MyNode("node12", IRunnable::READY));

    tasks[4].after({tasks[0], tasks[1]});
    tasks[6].after({tasks[2], tasks[3]});
    tasks[8].after({tasks[4], tasks[5]});
    tasks[9].after({tasks[5], tasks[6], tasks[7], tasks[10]});
    tasks[11].after({tasks[8], tasks[9]});
    graph.start();

    auto retrievedNode = graph.getNextNode(tasks[7].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    auto nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node6" || nodeName == "node4" || nodeName == "node11");

    retrievedNode = graph.getNextNode(tasks[0].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node6" || nodeName == "node4");

    retrievedNode = graph.getNextNode(tasks[5].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node6");

    retrievedNode = graph.getNextNode(tasks[2].getNode());
    EXPECT_EQ(retrievedNode.first, ThreadGraph::NodeExecutionState::READY);
    nodeName = static_cast<MyNode*>(retrievedNode.second)->_name;
    EXPECT_TRUE(nodeName == "node4");
}
