#pragma once

#include <atomic>
#include <iostream>

namespace perma {

class ThreadManager {
 public:
  static ThreadManager* get_manager();
  void reset(int number_threads);
  void wait_for_all_threads();
  void notify_thread_complete();

 private:
  ThreadManager() { std::cout << "Created ThreadManager Singleton" << std::endl; };  // = default;
  static ThreadManager* manager_;
  std::atomic<int> threads_alive_{};

 public:
  ThreadManager(ThreadManager const&) = delete;
  void operator=(ThreadManager const&) = delete;
};

}  // namespace perma
