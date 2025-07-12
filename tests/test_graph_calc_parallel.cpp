#include <gtest/gtest.h>

#include "athread/athread.h"

using namespace at;
using namespace std;

class CalcNode : public INode
{
public:
    CalcNode(double val = 0.0) : result(val) {}
    double result;
    void execute() override {}
};

class AdditionNode : public CalcNode
{
public:
    AdditionNode() {}
    void execute() override
    {
        double tempResult = 0.0;
        for (const auto& dep : predecessors())
        {
            auto cal = dynamic_cast<CalcNode*>(dep);
            if (cal)
            {
                tempResult += cal->result;
            }
        }
        result = tempResult;
        AT_COUT("Addition result: " << result << endl);
    }
};

class SubtractionNode : public CalcNode
{
public:
    SubtractionNode() {}
    void execute() override
    {
        double tempResult = 0.0;
        for (const auto& dep : predecessors())
        {
            auto cal = dynamic_cast<CalcNode*>(dep);
            if (cal)
            {
                tempResult -= cal->result;
            }
        }
        result = tempResult;
        AT_COUT("Subtraction result: " << result << endl);
    }
};

TEST(ThreadGraph, CalcParallel)
{
    ThreadGraph graph(2);
    auto task1 = graph.push(new CalcNode(5.2));
    auto task2 = graph.push(new CalcNode(3.8));
    auto task3 = graph.push(new CalcNode(2.4));
    auto task4 = graph.push(new CalcNode(3.6));
    auto task5 = graph.push(new CalcNode(8.2));
    auto task6 = graph.push(new CalcNode(2.1));
    auto task7 = graph.push(new CalcNode(4.6));

    auto addNode = graph.push(new AdditionNode());
    auto subNode = graph.push(new SubtractionNode());
    auto addNode2 = new AdditionNode();
    auto adding_task_2 = graph.push(addNode2);

    addNode.depend({task1, task2, task3, task4});
    subNode.depend({task5, task6, task7});
    adding_task_2.depend({addNode, subNode});

    graph.start();
    graph.wait();

    // Check results
    auto resultNode = dynamic_cast<const CalcNode*>(addNode2);
    ASSERT_NE(resultNode, nullptr);
    double expectedResult = ((double)5.2 + 3.8 + 2.4 + 3.6) + ((double)0.0 - 8.2 - 2.1 - 4.6);
    EXPECT_DOUBLE_EQ(resultNode->result, expectedResult);
}