#include "io_operation.hpp"

#include <iostream>

#include "read_write_ops.hpp"

namespace perma {

uint32_t Pause::get_length() const { return length_; }

void Pause::run() { std::this_thread::sleep_for(std::chrono::microseconds(length_)); }

void Read::run() { return rw_ops::simd_read(op_addresses_, access_size_); }

void Write::run() { return rw_ops::simd_write(op_addresses_, access_size_); }

}  // namespace perma
