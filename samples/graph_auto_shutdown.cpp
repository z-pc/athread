#include "athread/athread.h"
#include <iostream>
#include <chrono>

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;

    // Push a long-running task
    auto longTask = graph.push([]()
                               {
                                   cout << "Long task running..." << endl;
                                   std::this_thread::sleep_for(std::chrono::seconds(5));
                                   cout << "Long task completed!" << endl;
                               });

    // Start the graph
    graph.start();

    // Simulate an early shutdown
    std::this_thread::sleep_for(std::chrono::seconds(2));
    graph.terminate(true);

    cout << "Graph terminated before completion!" << endl;
    return 0;
}
