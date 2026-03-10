#include "thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
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

  auto serial_start = std::chrono::steady_clock::now();
  int expected = count_primes(0, kTotal);
  auto serial_elapsed = std::chrono::steady_clock::now() - serial_start;

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

  if (result.load() != expected) {
    std::cerr << "failed (expected: " << expected << ", got: " << result.load()
              << ")\n";
    return 1;
  }
  std::cout << "success (primes: " << result.load() << ")\n"
            << "  serial:   " << ms(serial_elapsed) << "ms\n"
            << "  parallel: " << ms(parallel_elapsed) << "ms\n";
  return 0;
}