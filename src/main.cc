#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

template <typename T> class LockFreeQueue {
public:
  explicit LockFreeQueue(int32_t size)
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
        val = slot.data;
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
  size_t size_;
  std::vector<Slot> buffer_;
};

int TestBufferRing() {
  constexpr int kTotal = 1000000;
  constexpr int kProducers = 7;
  constexpr int kConsumers = 13;
  LockFreeQueue<int> rb(4096);
  std::atomic<int> sum_produced{0};
  std::atomic<int> sum_consumed{0};
  std::atomic<int> consumed_count{0};
  std::vector<std::thread> producers;
  for (int t = 0; t < kProducers; ++t) {
    producers.emplace_back([&, t]() {
      for (int i = t; i < kTotal; i += kProducers) {
        while (!rb.Push(std::move(i)))
          ;
        sum_produced += i;
      }
    });
  }
  std::vector<std::thread> consumers;
  for (int t = 0; t < kConsumers; ++t) {
    consumers.emplace_back([&]() {
      int val;
      while (consumed_count.load() < kTotal) {
        if (rb.Pop(val)) {
          sum_consumed += val;
          ++consumed_count;
        }
      }
    });
  }
  for (auto &p : producers) {
    p.join();
  }
  for (auto &c : consumers) {
    c.join();
  }
  return sum_produced == sum_consumed;
}

int main(int argc, char *argv[]) {
  if (TestBufferRing()) {
    std::cout << "success!\n";
  }
  return 0;
}