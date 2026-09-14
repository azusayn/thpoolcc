#include "thpool_mutex.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  constexpr int kWorkers = 16;
  constexpr int kProducers = 32;
  constexpr int kTasks = 5'000'000;

  azusayn::ThreadPool pool(kWorkers);
  std::atomic<int> counter{0};

  auto start = std::chrono::steady_clock::now();

  std::vector<std::thread> producers;
  producers.reserve(kProducers);
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p]() {
      for (int i = p; i < kTasks; i += kProducers) {
        while (!pool.Submit([&counter]() {
          counter.fetch_add(1, std::memory_order_relaxed);
        })) {
          std::this_thread::yield();
        }
      }
    });
  }
  for (auto &t : producers) {
    t.join();
  }
  pool.Wait();

  auto elapsed = std::chrono::steady_clock::now() - start;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  const double tasks_per_sec =
      ms > 0 ? (static_cast<double>(kTasks) * 1000.0) / static_cast<double>(ms)
             : 0.0;

  if (counter.load() != kTasks) {
    std::cerr << "failed (expected: " << kTasks << ", got: " << counter.load()
              << ")\n";
    return 1;
  }
  std::cout << "success (tasks: " << counter.load() << ")\n"
            << "  wall:       " << ms << "ms\n"
            << "  throughput: " << static_cast<std::uint64_t>(tasks_per_sec)
            << " tasks/s\n";
  return 0;
}
