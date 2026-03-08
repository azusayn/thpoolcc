#include "thpoolcc.h"
#include <atomic>
#include <iostream>

void *f(void *args) {
  auto count_ptr = (std::atomic<int32_t> *)(args);
  for (int i = 0; i < 100000000; i++) {
    count_ptr->fetch_add(1);
  }
  return nullptr;
}

int main(int argc, char *argv[]) {
  ThreadPoolCC::ThreadPool thp(3);
  std::atomic<int32_t> count(0);
  for (int i = 0; i < 4; i++) {
    thp.AddWork(f, (void *)&count);
  }
  thp.Wait();
  std::cout << count << "\n";
  return 0;
}