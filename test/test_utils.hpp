#pragma once

#include <filesystem>

namespace perma {

#define _PRINT_JSON(json) "Got JSON:\n" << std::setw(2) << json
#define ASSERT_JSON_TRUE(json, assertion) ASSERT_TRUE(json.assertion) << _PRINT_JSON(json)
#define ASSERT_JSON_FALSE(json, assertion) ASSERT_FALSE(json.assertion) << _PRINT_JSON(json)
#define ASSERT_JSON_EQ(json, actual, expected) ASSERT_EQ(json.actual, expected) << _PRINT_JSON(json)

void check_file_written(const std::filesystem::path& pmem_file, size_t total_size);

}  // namespace perma
