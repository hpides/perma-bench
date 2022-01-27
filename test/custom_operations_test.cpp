#include "gtest/gtest.h"
#include "io_operation.hpp"

namespace perma {

class CustomOperationTest : public ::testing::Test {};

// Read Operations
TEST_F(CustomOperationTest, ParseCustomRead64) {
  CustomOp op = CustomOp::from_string("r_64");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Read, .is_pmem = true, .size = 64}));
}

TEST_F(CustomOperationTest, ParseCustomRead256) {
  CustomOp op = CustomOp::from_string("r_256");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Read, .is_pmem = true, .size = 256}));
}

TEST_F(CustomOperationTest, ParseCustomReadPMem4096) {
  CustomOp op = CustomOp::from_string("rp_4096");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Read, .is_pmem = true, .size = 4096}));
}

TEST_F(CustomOperationTest, ParseCustomReadDram256) {
  CustomOp op = CustomOp::from_string("rd_256");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Read, .is_pmem = false, .size = 256}));
}

TEST_F(CustomOperationTest, ParseBadRead333) { EXPECT_THROW(CustomOp::from_string("r_333"), PermaException); }

TEST_F(CustomOperationTest, ParseBadReadTooShort) { EXPECT_THROW(CustomOp::from_string("r"), PermaException); }

TEST_F(CustomOperationTest, ParseBadReadMissingSize) { EXPECT_THROW(CustomOp::from_string("r_"), PermaException); }

TEST_F(CustomOperationTest, ParseBadReadBadUnderscore) { EXPECT_THROW(CustomOp::from_string("r_p"), PermaException); }

TEST_F(CustomOperationTest, ParseBadReadWhitespace) { EXPECT_THROW(CustomOp::from_string("r p"), PermaException); }

TEST_F(CustomOperationTest, CustomRead64String) {
  EXPECT_EQ((CustomOp{.type = Operation::Read, .is_pmem = true, .size = 64}).to_string(), "rp_64");
}

TEST_F(CustomOperationTest, CustomRead256String) {
  EXPECT_EQ((CustomOp{.type = Operation::Read, .is_pmem = true, .size = 256}).to_string(), "rp_256");
}

TEST_F(CustomOperationTest, CustomReadDram128String) {
  EXPECT_EQ((CustomOp{.type = Operation::Read, .is_pmem = false, .size = 128}).to_string(), "rd_128");
}

// Write Operations
TEST_F(CustomOperationTest, ParseCustomWrite128None) {
  CustomOp op = CustomOp::from_string("w_128_none");
  EXPECT_EQ(op,
            (CustomOp{.type = Operation::Write, .is_pmem = true, .size = 128, .persist = PersistInstruction::None}));
}

TEST_F(CustomOperationTest, ParseCustomWrite128NoCache) {
  CustomOp op = CustomOp::from_string("w_128_nocache");
  EXPECT_EQ(op,
            (CustomOp{.type = Operation::Write, .is_pmem = true, .size = 128, .persist = PersistInstruction::NoCache}));
}

TEST_F(CustomOperationTest, ParseCustomWrite128Cache) {
  CustomOp op = CustomOp::from_string("w_128_cache");
  EXPECT_EQ(op,
            (CustomOp{.type = Operation::Write, .is_pmem = true, .size = 128, .persist = PersistInstruction::Cache}));
}

TEST_F(CustomOperationTest, ParseCustomWrite128CacheInv) {
  CustomOp op = CustomOp::from_string("w_128_cacheinv");
  EXPECT_EQ(
      op, (CustomOp{
              .type = Operation::Write, .is_pmem = true, .size = 128, .persist = PersistInstruction::CacheInvalidate}));
}

TEST_F(CustomOperationTest, ParseCustomWritePmem256) {
  CustomOp op = CustomOp::from_string("wp_256_nocache");
  EXPECT_EQ(op,
            (CustomOp{.type = Operation::Write, .is_pmem = true, .size = 256, .persist = PersistInstruction::NoCache}));
}

TEST_F(CustomOperationTest, ParseCustomWriteDram256) {
  CustomOp op = CustomOp::from_string("wd_256_cacheinv");
  EXPECT_EQ(
      op,
      (CustomOp{
          .type = Operation::Write, .is_pmem = false, .size = 256, .persist = PersistInstruction::CacheInvalidate}));
}

TEST_F(CustomOperationTest, ParseCustomWritePmem128Offset) {
  CustomOp op = CustomOp::from_string("wp_128_nocache_64");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Write,
                          .is_pmem = true,
                          .size = 128,
                          .persist = PersistInstruction::NoCache,
                          .offset = 64}));
}

TEST_F(CustomOperationTest, ParseCustomWritePmem128NegativeOffset) {
  CustomOp op = CustomOp::from_string("wp_128_cache_-64");
  EXPECT_EQ(op, (CustomOp{.type = Operation::Write,
                          .is_pmem = true,
                          .size = 128,
                          .persist = PersistInstruction::Cache,
                          .offset = -64}));
}

TEST_F(CustomOperationTest, ParseBadWriteOffset) {
  EXPECT_THROW(CustomOp::from_string("w_128_none_333"), PermaException);
}

TEST_F(CustomOperationTest, ParseBadWrite333) { EXPECT_THROW(CustomOp::from_string("w_333_none"), PermaException); }

TEST_F(CustomOperationTest, ParseBadWriteTooShort) { EXPECT_THROW(CustomOp::from_string("w"), PermaException); }

TEST_F(CustomOperationTest, ParseBadWriteMissingSize) { EXPECT_THROW(CustomOp::from_string("w_"), PermaException); }

TEST_F(CustomOperationTest, ParseBadWriteMissingPersist) {
  EXPECT_THROW(CustomOp::from_string("w_64"), PermaException);
}

TEST_F(CustomOperationTest, ParseBadWriteMissingPersistWithUnderscore) {
  EXPECT_THROW(CustomOp::from_string("w_64_"), PermaException);
}

TEST_F(CustomOperationTest, ParseBadWriteBadUnderscore) { EXPECT_THROW(CustomOp::from_string("w_p"), PermaException); }

TEST_F(CustomOperationTest, ParseBadWriteWhitespace) { EXPECT_THROW(CustomOp::from_string("w p"), PermaException); }

TEST_F(CustomOperationTest, CustomWrite64NoCacheString) {
  CustomOp op{.type = Operation::Write, .is_pmem = true, .size = 64, .persist = PersistInstruction::NoCache};
  EXPECT_EQ(op.to_string(), "wp_64_nocache");
}

TEST_F(CustomOperationTest, CustomWrite128CacheString) {
  CustomOp op{.type = Operation::Write, .is_pmem = true, .size = 128, .persist = PersistInstruction::Cache};
  EXPECT_EQ(op.to_string(), "wp_128_cache");
}

TEST_F(CustomOperationTest, CustomWrite256CacheInvString) {
  CustomOp op{.type = Operation::Write, .is_pmem = true, .size = 256, .persist = PersistInstruction::CacheInvalidate};
  EXPECT_EQ(op.to_string(), "wp_256_cacheinv");
}

TEST_F(CustomOperationTest, CustomWrite4096NoneString) {
  CustomOp op{.type = Operation::Write, .is_pmem = true, .size = 4096, .persist = PersistInstruction::None};
  EXPECT_EQ(op.to_string(), "wp_4096_none");
}

TEST_F(CustomOperationTest, CustomDramWrite128CacheString) {
  CustomOp op{.type = Operation::Write, .is_pmem = false, .size = 128, .persist = PersistInstruction::Cache};
  EXPECT_EQ(op.to_string(), "wd_128_cache");
}

TEST_F(CustomOperationTest, CustomWrite128OffsetString) {
  CustomOp op{
      .type = Operation::Write, .is_pmem = false, .size = 128, .persist = PersistInstruction::Cache, .offset = 128};
  EXPECT_EQ(op.to_string(), "wd_128_cache_128");
}

TEST_F(CustomOperationTest, CustomWrite128NegativeOffsetString) {
  CustomOp op{
      .type = Operation::Write, .is_pmem = false, .size = 128, .persist = PersistInstruction::Cache, .offset = -64};
  EXPECT_EQ(op.to_string(), "wd_128_cache_-64");
}

TEST_F(CustomOperationTest, ValidChainDramPmemReadWrite) {
  std::vector<CustomOp> ops = {CustomOp{.type = Operation::Read, .is_pmem = true},
                               CustomOp{.type = Operation::Write, .is_pmem = true},
                               CustomOp{.type = Operation::Write, .is_pmem = true},
                               CustomOp{.type = Operation::Read, .is_pmem = true},
                               CustomOp{.type = Operation::Read, .is_pmem = false},
                               CustomOp{.type = Operation::Write, .is_pmem = false},
  };
  EXPECT_TRUE(CustomOp::validate(ops));
}

TEST_F(CustomOperationTest, BadChainStartsWithWrite) {
  std::vector<CustomOp> ops = {CustomOp{.type = Operation::Write}, CustomOp{.type = Operation::Read}};
  EXPECT_FALSE(CustomOp::validate(ops));
}

TEST_F(CustomOperationTest, BadChainWithDramWriteAftrPmemRead) {
  std::vector<CustomOp> ops = {CustomOp{.type = Operation::Read, .is_pmem = true},
                               CustomOp{.type = Operation::Write, .is_pmem = false}};
  EXPECT_FALSE(CustomOp::validate(ops));
}

}  // namespace perma
