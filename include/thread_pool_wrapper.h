#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *NewThreadPool(uint32_t n_threads, uint32_t queue_size);

bool Submit(void *thpool, uintptr_t arg);

void Destroy(void *thpool);

void Wait(void *thpool);

#ifdef __cplusplus
}
#endif
