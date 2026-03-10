#include "lock_free_queue.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  constexpr int kTotal = 1000000;
  constexpr int kProducers = 7;
  constexpr int kConsumers = 13;
  azusayn::LockFreeQueue<int> queue(4096);
  std::atomic<int> sum_produced{0};
  std::atomic<int> sum_consumed{0};
  std::atomic<int> consumed_count{0};
  std::vector<std::thread> producers;

  auto start = std::chrono::steady_clock::now();

  for (int t = 0; t < kProducers; ++t) {
    producers.emplace_back([&, t]() {
      for (int i = t; i < kTotal; i += kProducers) {
        while (!queue.Push(std::move(i)))
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
        if (queue.Pop(val)) {
          sum_consumed += val;
          ++consumed_count;
        }
      }
    });
  }
  for (auto &p : producers)
    p.join();
  for (auto &c : consumers)
    c.join();

  auto elapsed = std::chrono::steady_clock::now() - start;
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

  if (sum_produced != sum_consumed) {
    std::cerr << "failed (produced: " << sum_produced
              << ", consumed: " << sum_consumed << ")\n";
    return 1;
  }
  std::cout << "success\n"
            << "  producers:  " << kProducers << "\n"
            << "  consumers:  " << kConsumers << "\n"
            << "  total:      " << kTotal << "\n"
            << "  elapsed:    " << ms << "ms\n";
  return 0;
}