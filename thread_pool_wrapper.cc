#include "thread_pool_wrapper.h"
#include "thread_pool.hpp"
#include <cstdint>

extern "C" {

extern void GoInvoke(uintptr_t);

void *NewThreadPool(uint32_t n_threads, uint32_t queueSize) {
  return new azusayn::ThreadPool(n_threads, queueSize);
}

bool Submit(void *thpool, uintptr_t arg) {
  return static_cast<azusayn::ThreadPool *>(thpool)->Submit(
      [arg]() { GoInvoke(arg); });
}

void Destroy(void *thpool) {
  static_cast<azusayn::ThreadPool *>(thpool)->Destroy();
}

void Wait(void *thpool) { static_cast<azusayn::ThreadPool *>(thpool)->Wait(); }
}