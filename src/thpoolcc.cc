// +--------------------------------+
// | Author:    azusaings@gmail.com |
// | License:	  GPLv3.0             |
// | Date:      2024.1.14           |
// +--------------------------------+ 
#include "thpoolcc.h"
// synchronized
#include <unistd.h>

#define DEBUG 1

#ifdef DEBUG
  #include <cstdio>
  #define THREAD_PRINT_DEBUG(msg) \
    printf("[tid-0x%lx]: %s\n", pthread_self(), msg)
#else
  #define THREAD_PRINT_DEBUG(msg)
#endif

namespace ThreadPoolCC {
  // routine for created pthreads
  static void* th_routine(void* arg) {
    THREAD_PRINT_DEBUG("init");
    ThreadPool* thp = static_cast<ThreadPool*>(arg);
    while(thp->thpool_alive_) { 
      // check whether the queue is not empty
      pthread_mutex_lock(&thp->mutex_work_buf_);
      while (!thp->n_works_ && thp->thpool_alive_) {
        // automatically get mutex lock when the condition is ready  
        pthread_cond_wait(&thp->cond_has_works_, &thp->mutex_work_buf_);
      }
      if(!thp->thpool_alive_) {
        pthread_mutex_unlock(&thp->mutex_work_buf_);
        break;
      }
      pthread_mutex_lock(&thp->mutex_thread_n_);
      thp->n_threads_working_++;
      pthread_mutex_unlock(&thp->mutex_thread_n_);

      // get work from queue
      Work* cur_work = thp->work_queue_front_;
      thp->work_queue_front_ = thp->work_queue_front_->next;
      thp->n_works_--;
      pthread_mutex_unlock(&thp->mutex_work_buf_);
      
      // execute task
      cur_work->func_p(cur_work->arg);
      delete cur_work;

      pthread_mutex_lock(&thp->mutex_thread_n_);
      thp->n_threads_working_--;
      if(!thp->n_threads_working_){
        pthread_cond_signal(&thp->cond_idle_);
      }
      pthread_mutex_unlock(&thp->mutex_thread_n_);
    } // exit: while(!thp->n_works)

    THREAD_PRINT_DEBUG("exits");
    pthread_mutex_lock(&thp->mutex_thread_n_);
    thp->n_threads_alive_--;
    pthread_mutex_unlock(&thp->mutex_thread_n_);

    return nullptr;
  }

  // initialization
  ThreadPool::ThreadPool(uint32_t n_threads):
    n_threads_alive_(0),
    n_threads_working_(0), 
    n_threads_(n_threads), 
    mutex_thread_n_(PTHREAD_MUTEX_INITIALIZER),
    mutex_work_buf_(PTHREAD_MUTEX_INITIALIZER), 
    cond_has_works_(PTHREAD_COND_INITIALIZER), 
    cond_idle_(PTHREAD_COND_INITIALIZER),
    n_works_(0), 
    thpool_alive_(true) {
    // init work queue
    work_queue_front_ = new Work[1];
    work_queue_rear_ = work_queue_front_;
    // init threadpool
    pthreads_ = new pthread_t[(int)n_threads];
    for (size_t i = 0; i < n_threads; i++) {
      pthread_create(pthreads_ + i, NULL, th_routine, (void*)this);
      // so the stack memory will be recycled by kernel after 'return'
      pthread_detach(pthreads_[i]); 
      n_threads_alive_++;
    }
  }
  
  // add works to work queue, return false if failed
  bool ThreadPool::AddWork(void* (*func_p)(void*), void* arg) {
    if(!thpool_alive_) {
      return false;
    }
    pthread_mutex_lock(&mutex_work_buf_);
    work_queue_rear_->func_p = func_p;
    work_queue_rear_->arg = arg;
    work_queue_rear_->next = new Work [1];
    work_queue_rear_ = work_queue_rear_->next;
    n_works_++;
    pthread_cond_signal(&cond_has_works_);
    pthread_mutex_unlock(&mutex_work_buf_);
    return true;
  }

  // stop thread pool
  void ThreadPool::Destroy() {
    this->thpool_alive_ = false;

    uint32_t n_threads_to_cleanup = n_threads_alive_;
    time_t now = time(nullptr);
    // wake up all the idle threads ASAP
    while(n_threads_alive_) {
      if(difftime(time(nullptr), now) >= 1) {
        break;
      }
      pthread_cond_broadcast(&cond_has_works_);
    }

    // then broadcast signal once a second
    while(n_threads_alive_) {
      pthread_cond_broadcast(&cond_has_works_);
      sleep(1);
    }
    THREAD_PRINT_DEBUG("all threads exit... after ThreadPool::destroy()");

    // resource recycling
    delete[] pthreads_;
    
    Work* curr = work_queue_front_;
    while(curr != nullptr) {
      Work* next = curr->next;
      delete[] curr;
      curr = next;
    }
    return;
  }

  // wait till all threads are idle and the work queue is empty
  void ThreadPool::Wait(uint8_t n_max_poll_secs=1) {
    uint8_t n_poll_secs = 1;
    pthread_mutex_lock(&mutex_thread_n_);
    while (n_threads_working_ || n_works_) {
      pthread_cond_wait(&cond_idle_, &mutex_thread_n_);
      sleep(n_poll_secs);
      // sleep for [1, 2, 4, ... , n_max_poll_secs] each time
      if(n_poll_secs * 2 < n_max_poll_secs) {
        n_poll_secs *= 2; 
      } else{
        n_poll_secs = n_max_poll_secs;
      }
    }
    pthread_mutex_unlock(&mutex_thread_n_);
    THREAD_PRINT_DEBUG("wake up from ThreadPool::wait()");
    return;
  }

} // namespace ThreadPoolCC 
