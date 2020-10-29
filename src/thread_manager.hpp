#pragma once

#include <atomic>
#include <chrono>
#include <vector>

namespace perma {

namespace internal {

struct Measurement {
  const std::chrono::high_resolution_clock::time_point start_ts;
  const std::chrono::high_resolution_clock::time_point end_ts;
};

}  // namespace internal
class ThreadManager {
 public:
  explicit ThreadManager(int number_threads) : threads_alive_(number_threads), results_(number_threads){};
  void wait_for_all_threads();
  void notify_thread_complete();

  std::vector<std::vector<internal::Measurement>> results_;

 private:
  std::atomic<int> threads_alive_{};
};

}  // namespace perma
