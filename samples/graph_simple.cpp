#include <iostream>

#include "athread/athread.h"

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;

    // Push simple tasks
    auto t1 = graph.push([]() { cout << "Task 1 executing" << endl; });
    auto t2 = graph.push([]() { cout << "Task 2 executing" << endl; });

    // Add dependency: t2 depends on t1
    t2.depend(t1);

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    cout << "All tasks completed!" << endl;
    return 0;
}
