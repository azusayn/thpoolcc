// +--------------------------------+
// | Author:    azusaings@gmail.com |
// | License:	  GPLv3.0             |
// | Date:      2024.1.14           |
// +--------------------------------+ 
#ifndef __THREAD_POOL_CC_H
#define __THREAD_POOL_CC_H

#include <stdint.h>
#include <pthread.h>
#include <mutex>

namespace ThreadPoolCC {

struct Work {
  Work* next;
  void* (*func_p)(void*);
  void* arg;
};

static void* th_routine(void* routine_arg);

class ThreadPool {
public:
  // thpool status
  volatile uint32_t n_works_;
  volatile uint32_t n_threads_;
  volatile uint32_t n_threads_alive_;
  volatile uint32_t n_threads_working_;
  bool              thpool_alive_;
  // internal data structure
  Work*             work_queue_front_;
  Work*             work_queue_rear_;
  // threads and sychronization
  pthread_mutex_t   mutex_work_buf_;
  pthread_mutex_t   mutex_thread_n_;
  pthread_cond_t    cond_has_works_;
  pthread_cond_t    cond_idle_;

  pthread_t*        pthreads_;
  
  ThreadPool(uint32_t n_threads);

  bool AddWork(void* (*func_p)(void*), void* arg);

  void Destroy();

  void Wait(uint8_t n_max_poll_secs);

};

}; // namespace ThreadPoolCC


#endif