#include "lock_free_queue.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>

// Stage 1: single-thread baseline. Same function payload as TaskQueue.
struct Task {
  std::function<void()> func;
};

int main() {
  constexpr int kTotal = 5'000'000;
  azusayn::LockFreeQueue<Task> queue(1u << 20);
  std::atomic<int> counter{0};

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kTotal; ++i) {
    while (!queue.Push(Task{[&counter]() {
      counter.fetch_add(1, std::memory_order_relaxed);
    }}))
      ;
    Task task;
    while (!queue.Pop(task))
      ;
    task.func();
  }
  auto elapsed = std::chrono::steady_clock::now() - start;
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  const double ops_per_sec =
      ms > 0 ? (static_cast<double>(kTotal) * 1000.0) / static_cast<double>(ms)
             : 0.0;

  if (counter.load() != kTotal) {
    std::cerr << "failed (expected: " << kTotal << ", got: " << counter.load()
              << ")\n";
    return 1;
  }
  std::cout << "success (LockFreeQueue<Task>, single-thread baseline)\n"
            << "  total:      " << kTotal << "\n"
            << "  wall:       " << ms << "ms\n"
            << "  throughput: " << static_cast<std::uint64_t>(ops_per_sec)
            << " ops/s\n";
  return 0;
}
