#include "utils.hpp"

#include <libpmem.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <random>

namespace perma {

char* map_pmem_file(const std::filesystem::path& file, const size_t expected_length) {
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), 0, 0, 0, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not map file: " + file.string()};
  }

  if (mapped_length != expected_length) {
    throw std::runtime_error("Existing pmem data file has wrong size.");
  }

  if (!is_pmem) {
    spdlog::warn("File {} is not in persistent memory!", file.string());
  }

  return static_cast<char*>(pmem_addr);
}

char* create_pmem_file(const std::filesystem::path& file, const size_t length) {
  int is_pmem;
  size_t mapped_length;
  void* pmem_addr = pmem_map_file(file.c_str(), length, PMEM_FILE_CREATE, 0644, &mapped_length, &is_pmem);
  if (pmem_addr == nullptr || (unsigned long)pmem_addr == 0xFFFFFFFFFFFFFFFF) {
    throw std::runtime_error{"Could not create file: " + file.string()};
  }

  if (!is_pmem) {
    spdlog::warn("File {} is not in persistent memory!", file.string());
  }

  if (length != mapped_length) {
    throw std::runtime_error{"Mapped size different than specified size!"};
  }

  return static_cast<char*>(pmem_addr);
}

std::filesystem::path generate_random_file_name(const std::filesystem::path& base_dir) {
  if (!std::filesystem::exists(base_dir)) {
    if (!std::filesystem::create_directories(base_dir)) {
      throw std::runtime_error{"Could not create dir: " + base_dir.string()};
    }
  }
  std::string str("abcdefghijklmnopqrstuvwxyz");
  std::random_device rd;
  std::mt19937 generator(rd());
  std::shuffle(str.begin(), str.end(), generator);
  const std::string file_name = str + ".file";
  const std::filesystem::path file{file_name};
  return base_dir / file;
}

uint64_t duration_to_nanoseconds(const std::chrono::high_resolution_clock::duration duration) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

// FROM https://www.csee.usf.edu/~kchriste/tools/genzipf.c and
// https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
//===========================================================================
//=  Function to generate Zipf (power law) distributed random variables     =
//=    - Input: alpha and N                                                 =
//=    - Output: Returns with Zipf distributed random variable              =
//===========================================================================
uint64_t zipf(const double alpha, const uint64_t n) {
  static bool first = true;  // Static first time flag
  static double c = 0;       // Normalization constant
  static double* sum_probs;  // Pre-calculated sum of probabilities
  double z;                  // Uniform random number (0 < z < 1)
  int zipf_value;            // Computed exponential value to be returned

  // Compute normalization constant on first call only
  if (first) {
    for (uint64_t i = 1; i <= n; i++) {
      c = c + (1.0 / pow((double)i, alpha));
    }
    c = 1.0 / c;

    sum_probs = static_cast<double*>(malloc((n + 1) * sizeof(*sum_probs)));
    sum_probs[0] = 0;
    for (uint64_t i = 1; i <= n; i++) {
      sum_probs[i] = sum_probs[i - 1] + c / pow((double)i, alpha);
    }
    first = false;
  }

  // Pull a uniform random number (0 < z < 1)
  do {
    z = rand_val();
  } while ((z == 0) || (z == 1));

  // Map z to the value
  int low = 1, high = n, mid;
  do {
    mid = floor((low + high) / 2);
    if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
      zipf_value = mid;
      break;
    } else if (sum_probs[mid] >= z) {
      high = mid - 1;
    } else {
      low = mid + 1;
    }
  } while (low <= high);

  return zipf_value - 1;  // Subtract one to map to a range from 0 - (n - 1)
}

//=========================================================================
//= Multiplicative LCG for generating uniform(0.0, 1.0) random numbers    =
//=   - x_n = 7^5*x_(n-1)mod(2^31 - 1)                                    =
//=   - With x seeded to 1 the 10000th x value should be 1043618065       =
//=   - From R. Jain, "The Art of Computer Systems Performance Analysis," =
//=     John Wiley & Sons, 1991. (Page 443, Figure 26.2)                  =
//=========================================================================
double rand_val() {
  const long a = 16807;       // Multiplier
  const long m = 2147483647;  // Modulus
  const long q = 127773;      // m div a
  const long r = 2836;        // m mod a
  static long x = 1687248;    // Random int value
  long x_div_q;               // x divided by q
  long x_mod_q;               // x modulo q
  long x_new;                 // New x value

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0) {
    x = x_new;
  } else {
    x = x_new + m;
  }

  // Return a random value between 0.0 and 1.0
  return ((double)x / m);
}

}  // namespace perma
