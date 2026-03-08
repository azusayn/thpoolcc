# ThreadPool++

A thread pool implemented with POSIX APIs. May be re-implemented with the C++ standard library for cross-platform compatibility.

## build
```c++
  mkdir build && cd build
  cmake ..
  cmake --build .
```

## API
```c++
  // initialization
  ThreadPool(uint32_t n_threads);

  // add work with its arguments
  bool AddWork(void *(*func_p)(void *), void *arg);

  // destroy the thread pool and recycle all the resources
  void Destroy();
  
  // wait until the work queue is empty and all threads are idle
  void Wait();
```
