#include "gtest/gtest.h"

#include <chrono>

#include <iostream>
#include <chrono>

template <class Clock>
void
display_precision()
{
  typedef std::chrono::duration<double, std::nano> NS;
  NS ns = typename Clock::duration(1);
  std::cout << "PRECISION IN NS: " << ns.count() << " ns\n";
}

int main(int argc, char** argv) {
  display_precision<std::chrono::high_resolution_clock>();
//  ::testing::InitGoogleTest(&argc, argv);
//
//  return RUN_ALL_TESTS();
}
