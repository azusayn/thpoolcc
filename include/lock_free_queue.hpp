// +--------------------------------+
// | Author:    azusaings@gmail.com |
// | License:   GPLv3.0             |
// | Date:      2026.3.9            |
// +--------------------------------+
#ifndef __LOCK_FREE_QUEUE_H
#define __LOCK_FREE_QUEUE_H

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace azusayn {
// supports MPMC.
template <typename T> class LockFreeQueue {
public:
  explicit LockFreeQueue(uint32_t size)
      : buffer_(size), size_(size), head_(0), tail_(0) {
    if (size < 2 || size_ & (size_ - 1)) {
      std::cerr << "size must be a power of 2 and >= 2\n";
      std::abort();
    }
    for (size_t i = 0; i < size_; ++i) {
      buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
  }

  bool Push(T &&val) {
    for (;;) {
      auto tail = tail_.load(std::memory_order_relaxed);
      auto &slot = buffer_[idx(tail)];
      if (slot.sequence.load(std::memory_order_acquire) != tail) {
        return false;
      }
      if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
        slot.data = std::move(val);
        slot.sequence.store(tail + 1, std::memory_order_release);
        return true;
      }
    }
  }

  bool Pop(T &val) {
    for (;;) {
      auto head = head_.load(std::memory_order_relaxed);
      auto &slot = buffer_[idx(head)];
      if (slot.sequence.load(std::memory_order_acquire) != head + 1) {
        return false;
      }
      if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
        val = std::move(slot.data);
        slot.sequence.store(head + size_, std::memory_order_release);
        return true;
      }
    }
  }

private:
  struct Slot {
    T data;
    std::atomic<uint64_t> sequence;
  };

  inline uint64_t idx(uint64_t val) { return val & (size_ - 1); }

  std::atomic<uint64_t> head_;
  std::atomic<uint64_t> tail_;
  uint32_t size_;
  std::vector<Slot> buffer_;
};
} // namespace azusayn

#endif