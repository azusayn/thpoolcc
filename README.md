# ThreadPoolCC

A lock-free C++ thread pool with an MPMC queue, implemented using only the C++ standard library.

## Build
```c++
  mkdir build && cd build
  cmake ..
  cmake --build .
```

## API
```c++
  // initialization
  ThreadPool(uint32_t n_threads, uint32_t queueSize = 8192);

  // submit tasks
  bool Submit(std::function<void()> func);

  // destroy the thread pool and recycle all the resources
  void Destroy();
  
  // wait until the task queue is empty and all threads are idle
  void Wait();
```
