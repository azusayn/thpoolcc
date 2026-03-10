# ThreadPoolCC

A header-only, lock-free C++ thread pool with an MPMC queue, implemented using only the C++ standard library.

## Build

```c++
  mkdir build && cd build
  cmake ..
  cmake --build . --config Release
```

## API

```c++
  // initialization.
  ThreadPool(uint32_t n_threads, uint32_t queueSize = 8192);

  // submits a task (non-blocking), returns false if the queue is full. 
  bool Submit(std::function<void()> func);

  // destroys the thread pool and recycle all the resources.
  void Destroy();
  
  // waits until the task queue is empty and all threads are idle.
  void Wait();
```
