// +--------------------------------+
// | Author:    azusaings@gmail.com |
// | License:	GPLv3.0             |
// | Date:      2024.1.14           |
// +--------------------------------+
#ifndef __THREAD_POOL_CC_H
#define __THREAD_POOL_CC_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <pthread.h>
#include <stdint.h>
#include <thread>
#include <utility>
#include <vector>

#ifdef DEBUG
#include <cstdio>
#define THREAD_PRINT_DEBUG(msg) printf("[tid-0x%lx]: %s\n", reinterpret_cast<uintptr_t>(pthread_self()), msg)
#else
#define THREAD_PRINT_DEBUG(msg)
#endif

namespace azusayn {

class TaskQueue {
public:
  struct Task {
    std::function<void()> func;
  };

  TaskQueue()
      : dummy_head_(new TaskNode()), n_tasks_(0) {
    real_tail_ = dummy_head_;
    dummy_head_->next = nullptr;
  }

  ~TaskQueue() {
    auto curr = dummy_head_;
    for (;;) {
      if (curr == nullptr) {
        break;
      }
      auto next = curr->next;
      delete curr;
      curr = next;
    }
    n_tasks_ = 0;
  }

  bool Pop(Task *task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (task == nullptr || dummy_head_ == real_tail_) {
      return false;
    }
    *task = dummy_head_->next->task;
    auto old_head = dummy_head_;
    dummy_head_ = dummy_head_->next;
    --n_tasks_;
    delete old_head;
    return true;
  }

  void Push(Task &&task) {
    std::lock_guard<std::mutex> lock(mutex_);
    real_tail_->next = new TaskNode;
    real_tail_->next->task = std::move(task);
    real_tail_ = real_tail_->next;
    real_tail_->next = nullptr;
    ++n_tasks_;
  }

  uint64_t Size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return n_tasks_;
  }

private:
  struct TaskNode {
    Task task;
    TaskNode *next;
  };

  TaskNode *dummy_head_;
  TaskNode *real_tail_;
  std::mutex mutex_;
  uint64_t n_tasks_;
};

class ThreadPool {
public:
  ThreadPool(uint32_t n_threads)
      : n_threads_alive_(0), n_pending_(0), thpool_alive_(true) {
    for (size_t i = 0; i < n_threads; i++) {
      threads_.emplace_back(&ThreadPool::loop, this);
      n_threads_alive_++;
    }
  }

  ~ThreadPool() { Destroy(); }

  bool Submit(std::function<void()> func) {
    if (!thpool_alive_) {
      return false;
    }
    task_queue_.Push(TaskQueue::Task{func});
    n_pending_++;
    cv_has_tasks_.notify_one();
    return true;
  }

  void Destroy() {
    // if thread pool is already destroyed, return.
    if (!thpool_alive_.exchange(false)) {
      return;
    }

    // wake up all the idle threads ASAP.
    using clock = std::chrono::steady_clock;
    auto deadline = clock::now() + std::chrono::seconds(1);
    while (n_threads_alive_) {
      if (clock::now() >= deadline) {
        break;
      }
      cv_has_tasks_.notify_all();
    }

    // then broadcast signal once a second
    while (n_threads_alive_) {
      cv_has_tasks_.notify_one();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    for (auto &t : threads_) {
      t.join();
    }
    THREAD_PRINT_DEBUG("all threads exit... after ThreadPool::Destroy()");

    return;
  }

  // Wait waits for all the tasks are done and all the threads are idle.
  void Wait() {
    std::unique_lock<std::mutex> lock(cv_idle_mutex_);
    cv_idle_.wait(lock, [this]() {
      return n_pending_ == 0;
    });
  }

private:
  void loop() {
    THREAD_PRINT_DEBUG("thread initialized");
    for (;;) {
      std::unique_lock<std::mutex> has_tasks_lock(cv_has_tasks_mutex_);
      cv_has_tasks_.wait(has_tasks_lock, [this]() {
        return !thpool_alive_ || task_queue_.Size() > 0;
      });
      has_tasks_lock.unlock();
      if (!thpool_alive_) {
        break;
      }

      TaskQueue::Task t;
      if (!task_queue_.Pop(&t)) {
        continue;
      }
      t.func();

      --n_pending_;
      if (n_pending_ == 0) {
        std::unique_lock<std::mutex> idle_lock(cv_idle_mutex_);
        cv_idle_.notify_one();
        idle_lock.unlock();
      }
    }

    n_threads_alive_--;

    THREAD_PRINT_DEBUG("thread exits");

    return;
  }

  std::vector<std::thread> threads_;

  std::atomic<uint32_t> n_threads_alive_;
  std::atomic<uint32_t> n_pending_;
  std::atomic<bool> thpool_alive_;

  std::mutex cv_has_tasks_mutex_;
  std::condition_variable cv_has_tasks_;
  std::mutex cv_idle_mutex_;
  std::condition_variable cv_idle_;
  TaskQueue task_queue_;
};

}; // namespace azusayn

#endif