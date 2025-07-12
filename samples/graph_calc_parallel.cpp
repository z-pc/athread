#include "athread/athread.h"
#include <atomic>
#include <iostream>

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;
    std::atomic<int> result{0};

    // Push tasks for parallel computation
    auto t1 = graph.push([&result]() { result += 10; });
    auto t2 = graph.push([&result]() { result += 20; });
    auto t3 = graph.push([&result]() { result += 30; });

    // Add dependencies to ensure sequential execution
    t2.depend(t1);
    t3.depend(t2);

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    cout << "Final result: " << result.load() << endl;
    return 0;
}
