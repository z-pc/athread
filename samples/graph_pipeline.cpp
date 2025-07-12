#include "athread/athread.h"
#include <iostream>
#include <string>

using namespace at;
using namespace std;

int main()
{
    ThreadGraph graph;

    // Stage 1: Read data
    auto readData = graph.push(
        []()
        {
            cout << "Reading data..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });

    // Stage 2: Process data
    auto processData = graph.push(
        []()
        {
            cout << "Processing data..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        });

    // Stage 3: Write data
    auto writeData = graph.push(
        []()
        {
            cout << "Writing data..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });

    // Add dependencies to form a pipeline
    processData.depend(readData);
    writeData.depend(processData);

    // Start the graph and wait for completion
    graph.start();
    graph.wait();

    cout << "Pipeline completed!" << endl;
    return 0;
}
