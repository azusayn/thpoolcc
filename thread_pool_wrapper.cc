#include "thread_pool_wrapper.h"
#include "thread_pool.hpp"

extern "C" {
void *NewThreadPool(uint32_t n_threads, uint32_t queueSize) {
  return new azusayn::ThreadPool(n_threads, queueSize);
}

bool Submit(void *thpool, void (*func)(void)) {
  return ((azusayn::ThreadPool *)thpool)->Submit(func);
}

void Destroy(void *thpool) {
  static_cast<azusayn::ThreadPool *>(thpool)->Destroy();
}

void Wait(void *thpool) { static_cast<azusayn::ThreadPool *>(thpool)->Wait(); }
}