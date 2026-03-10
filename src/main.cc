#include "lock_free_queue.hpp"
#include "thread_pool.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int TestLockFreeQueue() {
  constexpr int kTotal = 1000000;
  constexpr int kProducers = 7;
  constexpr int kConsumers = 13;
  azusayn::LockFreeQueue<int> rb(4096);
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

void TestThreadPool() {
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAILED: " << msg << "\n";                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

  azusayn::ThreadPool pool(8);
  constexpr int kTasks = 200;

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

  int expected = count_primes(0, kTasks * 500);

  std::atomic<int> result{0};
  for (int i = 0; i < kTasks; ++i) {
    int lo = i * 500, hi = lo + 500;
    CHECK(pool.Submit([&result, lo, hi, &count_primes]() {
      result.fetch_add(count_primes(lo, hi), std::memory_order_relaxed);
    }),
          "submit failed");
  }

  pool.Wait();
  CHECK(result.load() == expected, "prime stress: result mismatch");
  std::cout << "prime stress: passed (primes found: " << result.load() << ")\n";
}

int main(int argc, char *argv[]) {
  TestThreadPool();
  return 0;
}