#include "thread_pool_wrapper.h"
#include "thread_pool.hpp"
#include <cstdint>

extern "C" {

void *NewThreadPool(uint32_t n_threads, uint32_t queueSize) {
  if ((n_threads <= 0) || queueSize < 2 || (queueSize & (queueSize - 1))) {
    return nullptr;
  }
  return new azusayn::ThreadPool(n_threads, queueSize);
}

bool Submit(void *thpool, void (*func)(uintptr_t), uintptr_t arg) {
  return static_cast<azusayn::ThreadPool *>(thpool)->Submit(
      [func, arg]() { func(arg); });
}

void Destroy(void *thpool) {
  static_cast<azusayn::ThreadPool *>(thpool)->Destroy();
}

void Wait(void *thpool) { static_cast<azusayn::ThreadPool *>(thpool)->Wait(); }
}