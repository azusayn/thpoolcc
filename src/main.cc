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
  // basic submit and wait
  {
    azusayn::ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
      CHECK(pool.Submit([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
      }),
            "submit failed");
    }

    pool.Wait();
    CHECK(counter.load() == 100, "basic: counter should be 100");
    std::cout << "basic submit/wait: passed\n";
  }

  // multiple threads competing
  {
    azusayn::ThreadPool pool(8);
    std::atomic<int> sum{0};
    constexpr int kTotal = 10000;

    for (int i = 0; i < kTotal; ++i) {
      pool.Submit([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); });
    }

    pool.Wait();
    int expected = kTotal * (kTotal - 1) / 2;
    CHECK(sum.load() == expected, "mpmc: sum mismatch");
    std::cout << "mpmc stress: passed\n";
  }

  // submit after destroy should fail
  {
    azusayn::ThreadPool pool(2);
    pool.Destroy();
    CHECK(!pool.Submit([]() {}), "submit after destroy should fail");
    std::cout << "submit after destroy: passed\n";
  }

  // wait with no tasks should return immediately
  {
    azusayn::ThreadPool pool(2);
    auto start = std::chrono::steady_clock::now();
    pool.Wait();
    auto elapsed = std::chrono::steady_clock::now() - start;
    CHECK(elapsed < std::chrono::milliseconds(100),
          "wait with no tasks should be fast");
    std::cout << "wait with no tasks: passed\n";
  }

  std::cout << "all tests passed\n";
}

int main(int argc, char *argv[]) {
  TestThreadPool();
  return 0;
}