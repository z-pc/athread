#include <gtest/gtest.h>

#include "athread/task.h"
#include "athread/threadgraph.h"

using namespace at;

// Mock class for INode to simulate behavior
class MockINode : public INode
{
public:
    MockINode() = default;
    ~MockINode() override = default;
    void execute() override {}
};

// Test fixture for TaskIterator with ThreadGraph
class TaskIteratorWithGraphTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize ThreadGraph
        graph = new ThreadGraph(4);  // 4 worker threads

        // Create mock nodes and add them to the graph
        node1 = new MockINode();
        node2 = new MockINode();
        node3 = new MockINode();

        task1 = graph->push(node1);
        task2 = graph->push(node2);
        task3 = graph->push(node3);

        // Establish dependencies
        task2.depend(task1);  // task2 depends on task1
        task3.depend(task2);  // task3 depends on task2
    }

    void TearDown() override
    {
        delete graph;  // Clean up the graph
    }

    ThreadGraph* graph;
    INode *node1, *node2, *node3;
    Task task1, task2, task3;
};

// Test TaskIterator for iterating over predecessors
TEST_F(TaskIteratorWithGraphTest, PredecessorsIteration)
{
    auto it = task2.begin_predecessors();
    auto endIt = task2.end_predecessors();

    ASSERT_NE(it, endIt);  // Ensure there is at least one predecessor
    Task predecessor = *it;
    ASSERT_EQ(predecessor, task1);  // task1 is the only predecessor of task2
    ++it;
    ASSERT_EQ(it, endIt);  // Ensure no more predecessors
}

// Test TaskIterator for iterating over successors
TEST_F(TaskIteratorWithGraphTest, SuccessorsIteration)
{
    auto it = task2.begin_successors();
    auto endIt = task2.end_successors();

    ASSERT_NE(it, endIt);  // Ensure there is at least one successor
    Task successor = *it;
    ASSERT_EQ(successor, task3);  // task3 is the only successor of task2
    ++it;
    ASSERT_EQ(it, endIt);  // Ensure no more successors
}

// Test TaskIterator for empty predecessors
TEST_F(TaskIteratorWithGraphTest, EmptyPredecessors)
{
    auto it = task1.begin_predecessors();
    auto endIt = task1.end_predecessors();

    ASSERT_EQ(it, endIt);  // task1 has no predecessors
}

// Test TaskIterator for empty successors
TEST_F(TaskIteratorWithGraphTest, EmptySuccessors)
{
    auto it = task3.begin_successors();
    auto endIt = task3.end_successors();

    ASSERT_EQ(it, endIt);  // task3 has no successors
}

// Test TaskIterator operators in a real DAG
TEST_F(TaskIteratorWithGraphTest, IteratorOperators)
{
    auto it = task2.begin_predecessors();
    ASSERT_NE(it, task2.end_predecessors());  // Ensure iterator is valid

    Task task = *it;  // Test dereference operator
    ASSERT_EQ(task, task1);

    Task* taskPtr = it.operator->();  // Test member access operator
    ASSERT_NE(taskPtr, nullptr);
    ASSERT_EQ(*taskPtr, task1);

    ++it;  // Test pre-increment operator
    ASSERT_EQ(it, task2.end_predecessors());

    it = task2.begin_predecessors();
    auto tempIt = it++;  // Test post-increment operator
    ASSERT_EQ(*tempIt, task1);
    ASSERT_EQ(it, task2.end_predecessors());
}
