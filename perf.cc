#include "lock_free_queue.hpp"
#include "thread_pool.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

void TestLockFreeQueue() {
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
    return;
  }
  std::cout << "success\n"
            << "  producers:  " << kProducers << "\n"
            << "  consumers:  " << kConsumers << "\n"
            << "  total:      " << kTotal << "\n";
}

void TestThreadPool() {
  azusayn::ThreadPool pool(16);
  constexpr int kTasks = 16;
  constexpr int kTotal = 100000000;
  constexpr int kChunk = kTotal / kTasks;

  auto count_primes = [](int lo, int hi) -> int {
    int cnt = 0;
    for (int n = lo; n < hi; ++n) {
      if (n < 2)
        continue;
      bool ok = true;
      for (int d = 2; d * d <= n; ++d)
        if (n % d == 0) {
          ok = false;
          break;
        }
      if (ok)
        ++cnt;
    }
    return cnt;
  };

  // serial
  auto serial_start = std::chrono::steady_clock::now();
  int expected = count_primes(0, kTotal);
  auto serial_elapsed = std::chrono::steady_clock::now() - serial_start;

  // parallel
  std::atomic<int> result{0};
  auto parallel_start = std::chrono::steady_clock::now();
  for (int t = 0; t < kTasks; ++t) {
    int lo = t * kChunk;
    int hi = (t == kTasks - 1) ? kTotal : lo + kChunk;
    while (!pool.Submit([lo, hi, &result, &count_primes]() {
      result.fetch_add(count_primes(lo, hi), std::memory_order_relaxed);
    }))
      std::this_thread::yield();
  }
  pool.Wait();
  auto parallel_elapsed = std::chrono::steady_clock::now() - parallel_start;

  auto ms = [](auto d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  };

  if (result.load() != expected)
    std::cerr << "failed (expected: " << expected << ", got: " << result.load()
              << ")\n";
  else
    std::cout << "success (primes: " << result.load() << ")\n"
              << "  serial:   " << ms(serial_elapsed) << "ms\n"
              << "  parallel: " << ms(parallel_elapsed) << "ms\n";
}

int main(int argc, char *argv[]) {
  std::cout << "+--------------------------- TestLockFreeQueue()\n";
  TestLockFreeQueue();
  std::cout << "+--------------------------- TestThreadPool()\n";
  TestThreadPool();
  return 0;
}