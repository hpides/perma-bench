#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <string>

#include "benchmark.hpp"

namespace perma {

std::vector<Benchmark> BenchmarkFactory::create_benchmarks(const std::filesystem::path& pmem_directory,
                                                           const std::filesystem::path& config_file_path) {
  std::vector<Benchmark> benchmarks{};
  std::vector<std::filesystem::path> config_files{};
  if (std::filesystem::is_directory(config_file_path)) {
    for (const std::filesystem::path& config_file : std::filesystem::directory_iterator(config_file_path)) {
      if (config_file.extension() == internal::CONFIG_FILE_EXTENSION) {
        config_files.push_back(config_file);
      }
    }
  } else {
    config_files.push_back(config_file_path);
  }

  for (const std::filesystem::path& config_file : config_files) {
    try {
      YAML::Node config = YAML::LoadFile(config_file);
      for (YAML::iterator it = config.begin(); it != config.end(); ++it) {
        const auto name = it->first.as<std::string>();

        // Check if config contains a nested 'args'
        YAML::Node raw_bm_args = it->second;
        YAML::Node bm_args = raw_bm_args["args"];
        if (!bm_args && raw_bm_args) {
          throw std::invalid_argument{"Benchmark config must contain 'args' if it is not empty."};
        }

        YAML::Node bm_matrix = raw_bm_args["matrix"];
        if (bm_matrix) {
          std::vector<Benchmark> matrix = create_benchmark_matrix(name, pmem_directory, bm_args, bm_matrix);
          std::move(matrix.begin(), matrix.end(), std::back_inserter(benchmarks));
        } else {
          BenchmarkConfig bm_config = BenchmarkConfig::decode(bm_args);
          bm_config.pmem_directory = pmem_directory;
          benchmarks.emplace_back(name, bm_config);
        }
      }
    } catch (const YAML::ParserException& e1) {
      throw std::runtime_error{"Exception during config parsing: " + e1.msg};
    } catch (const YAML::BadFile& e2) {
      throw std::runtime_error{"Exception during config parsing: " + e2.msg};
    }
  }
  return benchmarks;
}

std::vector<Benchmark> BenchmarkFactory::create_benchmark_matrix(const std::string& bm_name,
                                                                 const std::filesystem::path& pmem_directory,
                                                                 YAML::Node& config_args, YAML::Node& matrix_args) {
  if (!matrix_args.IsMap()) {
    throw std::invalid_argument("'matrix' must be a YAML map.");
  }

  const std::filesystem::path pmem_data_file = generate_random_file_name(pmem_directory);

  std::vector<Benchmark> matrix{};
  std::set<std::string> matrix_arg_names{};

  std::function<void(const YAML::iterator&, YAML::Node&)> create_matrix = [&](const YAML::iterator& node_iterator,
                                                                              YAML::Node& current_config) {
    YAML::Node current_values = node_iterator->second;
    if (node_iterator == matrix_args.end() || current_values.IsNull()) {
      // End of matrix recursion
      // We need to copy here to keep the tags clean in the YAML.
      // Otherwise everything is 'visited' after the first iteration and decoding fails.
      YAML::Node clean_config = YAML::Clone(current_config);

      BenchmarkConfig final_config = BenchmarkConfig::decode(clean_config);
      final_config.pmem_directory = pmem_directory;
      final_config.matrix_args = {matrix_arg_names.begin(), matrix_arg_names.end()};

      // Generate unique file for benchmarks that write or reuse existing file for read-only benchmarks.
      if (final_config.write_ratio > 0) {
        matrix.emplace_back(bm_name, final_config);
      } else {
        matrix.emplace_back(bm_name, final_config, pmem_data_file);
      }

      return;
    }

    if (!current_values.IsSequence()) {
      throw std::invalid_argument("Matrix entries must be a YAML sequence, i.e., [a, b, c].");
    }

    const auto arg_name = node_iterator->first.as<std::string>();
    matrix_arg_names.insert(arg_name);
    YAML::iterator next_node = node_iterator;
    next_node++;
    for (YAML::Node value : current_values) {
      current_config[arg_name] = value;
      create_matrix(next_node, current_config);
    }
  };

  YAML::Node base_config = YAML::Clone(config_args);
  create_matrix(matrix_args.begin(), base_config);
  return matrix;
}

}  // namespace perma
