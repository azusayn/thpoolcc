
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

template <typename T> class RingBuffer {
public:
  explicit RingBuffer(int32_t capacity)
      : buffer_(capacity + 1), size_(capacity + 1), head_(0), tail_(0) {}

  bool Push(T &&val) {
    if (idx(tail_ + 1) == idx(head_)) {
      return false;
    }
    buffer_[idx(tail_)] = val;
    tail_++;
    return true;
  }

  bool Pop(T &val) {
    if (idx(head_) == idx(tail_)) {
      return false;
    }
    val = buffer_[idx(head_)];
    head_++;
    return true;
  }

private:
  inline uint64_t idx(uint64_t val) { return val % size_; }

  std::atomic<uint64_t> head_;
  std::atomic<uint64_t> tail_;
  // total length of the underlying array.
  size_t size_;
  std::vector<T> buffer_;
};

int main(int argc, char *argv[]) {
  RingBuffer<std::string> rb(2);
  std::string names[] = {"bob", "alice", "tommy"};
  for (auto s : names) {
    auto ok = rb.Push(std::move(s));
    if (!ok) {
      std::cout << "failed to push '" + s + "' \n";
      continue;
    }
    std::cout << "success\n";
  }

  return 0;
}