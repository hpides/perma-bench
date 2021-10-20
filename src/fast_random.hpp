#pragma once

#include <cstdint>

/**
 * The code here is based on code from D. Lemire.
 * Original code at https://github.com/lemire/testingRNG/blob/master/source/lehmer64.h
 * Original code is licensed under Apache 2.0
 *
 * D. H. Lehmer, Mathematical methods in large-scale computing units.
 * Proceedings of a Second Symposium on Large Scale Digital Calculating
 * Machinery; Annals of the Computation Laboratory, Harvard Univ. 26 (1951), pp. 141-146.
 *
 * P L'Ecuyer,  Tables of linear congruential generators of different sizes and
 * good lattice structure. Mathematics of Computation of the American
 * Mathematical Society 68.225 (1999): 249-260.
 */

namespace perma {

extern thread_local __uint128_t g_lehmer64_state;

static inline uint64_t splitmix64_stateless(uint64_t index) {
  uint64_t z = (index + UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

static inline void lehmer64_seed(uint64_t seed) {
  g_lehmer64_state = (((__uint128_t)splitmix64_stateless(seed)) << 64) + splitmix64_stateless(seed + 1);
}

static inline uint64_t lehmer64() {
  g_lehmer64_state *= UINT64_C(0xda942042e4dd58b5);
  return g_lehmer64_state >> 64;
}

}  // namespace perma
