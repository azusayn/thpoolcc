// +--------------------------------+
// | Author:    azusaings@gmail.com |
// | License:   GPLv3.0             |
// | Date:      2024.1.14           |
// +--------------------------------+
#pragma once
#include "lock_free_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>

namespace azusayn {

class ThreadPool {
public:
  ThreadPool(uint32_t n_threads, uint32_t queueSize = 8192)
      : thpool_alive_(true), work_sem_(0), lock_free_queue_(queueSize) {
    for (size_t i = 0; i < n_threads; i++) {
      threads_.emplace_back(&ThreadPool::loop, this);
    }
  }

  ~ThreadPool() { Destroy(); }

  bool Submit(std::function<void()> func) {
    if (!thpool_alive_.load(std::memory_order_acquire)) {
      return false;
    }
    if (!lock_free_queue_.Push({std::move(func)})) {
      return false;
    }
    n_pending_.fetch_add(1, std::memory_order_release);
    work_sem_.release();
    return true;
  }

  void Destroy() {
    if (!thpool_alive_.exchange(false)) {
      return;
    }
    for (size_t i = 0; i < threads_.size(); i++) {
      work_sem_.release();
    }
    for (auto &t : threads_) {
      t.join();
    }
  }

  void Wait() {
    std::unique_lock<std::mutex> lock(cv_idle_mutex_);
    while (n_pending_.load(std::memory_order_relaxed)) {
      cv_idle_.wait(lock);
    }
  }

private:
  struct Task {
    std::function<void()> func;
  };

  void loop() {
    while (thpool_alive_.load(std::memory_order_acquire)) {
      work_sem_.acquire();
      if (!thpool_alive_.load(std::memory_order_acquire)) {
        break;
      }
      Task t;
      while (!lock_free_queue_.Pop(t)) {
        std::this_thread::yield();
      }
      t.func();
      if (n_pending_.fetch_sub(1, std::memory_order_release) == 1) {
        std::lock_guard<std::mutex> lock(cv_idle_mutex_);
        cv_idle_.notify_all();
      }
    }
  }

  LockFreeQueue<Task> lock_free_queue_;
  std::vector<std::thread> threads_;
  std::condition_variable cv_idle_;
  std::mutex cv_idle_mutex_;
  std::counting_semaphore<> work_sem_;
  std::atomic<bool> thpool_alive_;
  std::atomic<uint32_t> n_pending_;
};
} // namespace azusayn
