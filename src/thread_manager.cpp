#include "thread_manager.hpp"

namespace perma {

ThreadManager* ThreadManager::get_manager() {
  if (!manager_) {
    manager_ = new ThreadManager();
  }
  return manager_;
}

void ThreadManager::reset(int number_threads) { threads_alive_.store(number_threads); }

void ThreadManager::wait_for_all_threads() {}

void ThreadManager::notify_thread_complete() {}

}  // namespace perma
