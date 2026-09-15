#include "thpool_mutex.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

// Stage 2+3: multi-thread MPMC + producer scaling curve (one binary, many runs).
namespace {

struct Result {
  int producers;
  long long ms;
  double ops_per_sec;
};

Result RunOnce(int producers, int consumers, int total) {
  azusayn::TaskQueue queue;
  std::atomic<int> counter{0};
  std::atomic<int> consumed_count{0};

  auto start = std::chrono::steady_clock::now();

  std::vector<std::thread> producer_threads;
  producer_threads.reserve(producers);
  for (int p = 0; p < producers; ++p) {
    producer_threads.emplace_back([&, p]() {
      for (int i = p; i < total; i += producers) {
        queue.Push(azusayn::TaskQueue::Task{[&counter]() {
          counter.fetch_add(1, std::memory_order_relaxed);
        }});
      }
    });
  }

  std::vector<std::thread> consumer_threads;
  consumer_threads.reserve(consumers);
  for (int c = 0; c < consumers; ++c) {
    consumer_threads.emplace_back([&]() {
      azusayn::TaskQueue::Task task;
      while (consumed_count.load(std::memory_order_relaxed) < total) {
        if (queue.Pop(&task)) {
          task.func();
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for (auto &t : producer_threads)
    t.join();
  for (auto &t : consumer_threads)
    t.join();

  auto elapsed = std::chrono::steady_clock::now() - start;
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  if (counter.load() != total) {
    std::cerr << "failed at producers=" << producers
              << " (expected: " << total << ", got: " << counter.load()
              << ")\n";
    std::exit(1);
  }
  const double ops_per_sec =
      ms > 0 ? (static_cast<double>(total) * 1000.0) / static_cast<double>(ms)
             : 0.0;
  return Result{producers, ms, ops_per_sec};
}

} // namespace

int main() {
  constexpr int kTotal = 5'000'000;
  constexpr int kConsumers = 16;
  constexpr std::array<int, 8> kProducersList = {1, 2, 4, 8, 16, 32, 48, 64};

  std::cout << "TaskQueue MPMC scale (function payload)\n"
            << "  consumers: " << kConsumers << "\n"
            << "  total:     " << kTotal << "\n";

  for (int producers : kProducersList) {
    const Result r = RunOnce(producers, kConsumers, kTotal);
    std::cout << "  producers=" << r.producers << "  wall=" << r.ms
              << "ms  throughput=" << static_cast<std::uint64_t>(r.ops_per_sec)
              << " ops/s\n";
  }
  return 0;
}
