# Thread Pool & ThreadGraph

This library provides two main components: a traditional **ThreadPool** and an advanced **ThreadGraph** for multi-threaded task execution using a DAG (Directed Acyclic Graph) model.  
It helps you manage threads and tasks easily, safely, and efficiently.

## Requirements
- C++14 or later
- CMake 3.20+ to run examples

### Add to your project
You can either build the project as a static library and link it to your application, **or** simply copy the files in `src/*` into your project to use the thread pool and thread graph directly.

### Run examples
```sh
mkdir build && cd build
cmake ..
cmake --build .
./thread_pool
```

## ThreadPool Example

**Create a thread pool:**
```cpp
auto pool = ThreadPool(2, std::thread::hardware_concurrency(), 60s);
```

**Push tasks to the thread pool:**
```cpp
pool.emplace<RunnableExample>("#run 1000 miles#");
pool.emplace<RunnableExample>("#cooking breakfast#");
pool.emplace<RunnableExample>("#feed the cat#");
pool.emplace<RunnableExample>("#take out the trash#");
pool.emplace<RunnableExample>("#play chess with grandma#");
pool.emplace<RunnableExample>("#blah blah blah#");
pool.emplace<RunnableExample>("#marry...#");
```

**Notify the thread pool to stop:**
```cpp
pool.terminate();
```

**Wait until all workers actually finish:**
```cpp
pool.wait();
```

---

## ThreadGraph: Multi-threading with DAG Model

**ThreadGraph** allows you to model tasks and their dependencies as a Directed Acyclic Graph (DAG), enabling optimal parallel execution while respecting dependencies.

### How to use

**Declare a graph and result:**
```cpp
at::ThreadGraph graph;
std::atomic<int> result{0};
```

**Add nodes (tasks) to the graph:**
```cpp
auto t1 = graph.push([&result]() { result += 10; });
auto t2 = graph.push([&result]() { result += 20; });
auto t3 = graph.push([&result]() { result += 30; });
```

**Set dependencies between nodes:**
```cpp
t2.depend(t1); // t2 runs after t1
t3.depend(t2); // t3 runs after t2
```

**Start the graph and wait for completion:**
```cpp
graph.start();
graph.wait();
```

**Example output:**
```cpp
std::cout << "Final result: " << result.load() << std::endl;
```

**Alternatively, you can use `at::Executor` to run the ThreadGraph and obtain a `std::future` for asynchronous waiting or result retrieval:**
```cpp
at::Executor executor;
auto future = executor.run(graph);
future.wait(); // Wait for the graph to finish
```

### Advantages
- Automatically determines execution order based on dependencies.
- Maximizes CPU resource utilization.
- Easy to extend and maintain.
- Supports asynchronous execution and result retrieval via `std::future` when using `at::Executor`.

---

## Contribution

All feedback, bug reports, and pull requests are welcome!
