#include <iostream>
#include <vector>

#include "athread/athread.h"

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;
    std::vector<int> fib(10, 0);
    fib[0] = 0;
    fib[1] = 1;

    std::vector<Task> tasks(10);

    tasks[0] = graph.push([&fib]() { fib[0] = 0; });
    tasks[1] = graph.push([&fib]() { fib[1] = 1; });

    for (int i = 2; i < 10; ++i)
    {
        tasks[i] = graph.push(
            [i, &fib]()
            {
                fib[i] = fib[i - 1] + fib[i - 2];
                AT_COUT("Fib[" << i << "] = " << fib[i] << endl);
            });
        tasks[i].depend(tasks[i - 1]);
        tasks[i].depend(tasks[i - 2]);
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    cout << "Fibonacci sequence computed!" << endl;
    return 0;
}
