#pragma once

#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

void *NewThreadPool(uint32_t n_threads, uint32_t queueSize);

bool Submit(void *thpool, void (*func)(void));

void Destroy(void *thpool);

void Wait(void *thpool);

#ifdef __cplusplus
}
#endif
