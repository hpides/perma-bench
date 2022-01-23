#include "io_operation.hpp"

#include <sstream>
#include <string>

#include "spdlog/spdlog.h"

namespace perma {

CustomOp CustomOp::from_string(const std::string& str) {
  if (str.empty()) {
    spdlog::error("Custom operation cannot be empty!");
    utils::crash_exit();
  }

  CustomOp op;
  char type_char = str[0];
  if (type_char == 'r') {
    op.type = Operation::Read;
  } else if (type_char == 'w') {
    op.type = Operation::Write;
  } else {
    spdlog::error("Unknown type char: {}", type_char);
    utils::crash_exit();
  }

  const size_t delim_pos = str.find('_');
  std::string size_str = str.substr(1, delim_pos);
  op.size = std::stoul(size_str);

  if ((op.size & (op.size - 1)) != 0) {
    spdlog::error("Access size of custom operation must be power of 2. Got {}", op.size);
    utils::crash_exit();
  }

  if (op.type == Operation::Write) {
    if (delim_pos == str.length()) {
      spdlog::error("Custom write op must end with '_<persist_instruction>', e.g., w64_cache");
      utils::crash_exit();
    }
    std::string persist_str = str.substr(delim_pos + 1, str.length());
    if (persist_str == "none") {
      op.persist = PersistInstruction::None;
    } else if (persist_str == "cache") {
      op.persist = PersistInstruction::Cache;
    } else if (persist_str == "cache_inv") {
      op.persist = PersistInstruction::CacheInvalidate;
    } else if (persist_str == "nocache") {
      op.persist = PersistInstruction::NoCache;
    } else {
      spdlog::error("Could not parse the persist instruction in write op: '{}'", persist_str);
      utils::crash_exit();
    }
  }
  return op;
}

std::vector<CustomOp> CustomOp::all_from_string(const std::string& str) {
  if (str.empty()) {
    spdlog::error("Custom operations cannot be empty!");
    utils::crash_exit();
  }

  std::vector<CustomOp> ops;
  std::stringstream stream{str};
  std::string op_str;
  while (std::getline(stream, op_str, ',')) {
    ops.emplace_back(from_string(op_str));
  }

  if (ops[0].type != Operation::Read) {
    spdlog::error("First custom operation must be a read");
    utils::crash_exit();
  }

  return ops;
}

std::string CustomOp::to_string(const CustomOp& op) {
  std::stringstream out;
  char type_str = op.type == Operation::Read ? 'r' : 'w';
  out << type_str << op.size;
  if (op.type == Operation::Write) {
    out << '_';
    if (op.persist == PersistInstruction::None) {
      out << "none";
    } else if (op.persist == PersistInstruction::Cache) {
      out << "cache";
    } else if (op.persist == internal::PersistInstruction::CacheInvalidate) {
      out << "cache_inv";
    } else if (op.persist == internal::PersistInstruction::NoCache) {
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
