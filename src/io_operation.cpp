#include "io_operation.hpp"

#include <sstream>
#include <string>

#include "spdlog/spdlog.h"

namespace perma {

CustomOp CustomOp::from_string(const std::string& str) {
  if (str.empty()) {
    spdlog::error("Custom operation cannot be empty!");
    crash_exit();
  }

  CustomOp op;
  char type_char = str[0];
  if (type_char == 'r') {
    op.type = internal::Read;
  } else if (type_char == 'w') {
    op.type = internal::Write;
  } else {
    spdlog::error("Unknown type char: {}", type_char);
    crash_exit();
  }

  const size_t delim_pos = str.find('_');
  std::string size_str = str.substr(1, delim_pos);
  op.size = std::stoul(size_str);

  if ((op.size & (op.size - 1)) != 0) {
    spdlog::error("Access size of custom operation must be power of 2. Got {}", op.size);
    crash_exit();
  }

  if (op.type == internal::Write) {
    if (delim_pos == str.length()) {
      spdlog::error("Custom write op must end with '_<persist_instruction>', e.g., w64_cache");
      crash_exit();
    }
    std::string persist_str = str.substr(delim_pos + 1, str.length());
    if (persist_str == "none") {
      op.persist = internal::None;
    } else if (persist_str == "cache") {
      op.persist = internal::Cache;
    } else if (persist_str == "nocache") {
      op.persist = internal::NoCache;
    } else {
      spdlog::error("Could not parse persist instruction in '{}'", persist_str);
      crash_exit();
    }
  }
  return op;
}

std::vector<CustomOp> CustomOp::all_from_string(const std::string& str) {
  if (str.empty()) {
    spdlog::error("Custom operations cannot be empty!");
    crash_exit();
  }

  std::vector<CustomOp> ops;
  std::stringstream stream{str};
  std::string op_str;
  while (std::getline(stream, op_str, ',')) {
    ops.emplace_back(from_string(op_str));
  }

  if (ops[0].type != internal::Read) {
    spdlog::error("First custom operation must be a read");
    crash_exit();
  }

  return ops;
}

std::string CustomOp::to_string(const CustomOp& op) {
  std::stringstream out;
  char type_str = op.type == internal::Read ? 'r' : 'w';
  out << type_str << op.size;
  if (op.type == internal::Write) {
    out << '_';
    if (op.persist == internal::None) {
      out << "none";
    } else if (op.persist == internal::Cache) {
      out << "cache";
    } else if (op.persist == internal::NoCache) {
      out << "nocache";
    }
  }
  return out.str();
}

std::string CustomOp::all_to_string(const std::vector<CustomOp>& ops) {
  std::stringstream out;
  for (size_t i = 0; i < ops.size() - 1; ++i) {
    out << to_string(ops[i]) << ',';
  }
  out << to_string(ops[ops.size() - 1]);
  return out.str();
}

}  // namespace perma
