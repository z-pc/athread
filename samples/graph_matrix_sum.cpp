#include "athread/athread.h"
#include <atomic>
#include <iostream>
#include <numeric>
#include <vector>

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;
    std::vector<std::vector<int>> matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::atomic<int> totalSum{0};

    // Push tasks to compute the sum of each row
    for (const auto& row : matrix)
    {
        graph.push(
            [&row, &totalSum]()
            {
                int rowSum = std::accumulate(row.begin(), row.end(), 0);
                totalSum += rowSum;
            });
    }

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    cout << "Total sum of matrix: " << totalSum.load() << endl;
    return 0;
}
