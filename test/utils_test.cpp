#include "utils.hpp"

#include "gtest/gtest.h"

/**
 * Verifies whether a zipfian generated value is in between the given boundaries.
 */
TEST(UtilsTest, ZipfBound) {
  const uint64_t value = perma::zipf(0.99, 1000);
  EXPECT_GE(value, 0);
  EXPECT_LT(value, 1000);
}
