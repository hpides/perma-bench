#include "utils.hpp"

#include "gtest/gtest.h"

TEST(sample_test_case, sample_test) {
  const uint64_t value = perma::zipf(0.99, 1000);
  const bool is_between = 0 <= value && value < 1000;
  EXPECT_EQ(true, is_between);
}
