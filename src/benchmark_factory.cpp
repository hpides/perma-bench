#include "benchmark_factory.hpp"

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <string>

#include "benchmark.hpp"

namespace perma {

std::vector<UnaryBenchmark> BenchmarkFactory::create_single_benchmarks(const std::filesystem::path& pmem_directory,
                                                                       std::vector<YAML::Node>& configs) {
  std::vector<UnaryBenchmark> benchmarks{};

  for (YAML::Node& config : configs) {
    for (YAML::iterator it = config.begin(); it != config.end(); ++it) {
      const auto name = it->first.as<std::string>();
      YAML::Node raw_bm_args = it->second;

      // Ignore parallel benchmarks
      YAML::Node parallel_bm = raw_bm_args["parallel_benchmark"];
      if (parallel_bm.IsDefined()) {
        continue;
      }

      YAML::Node bm_args = raw_bm_args["args"];
      if (!bm_args && raw_bm_args) {
        throw std::invalid_argument{"Benchmark config must contain 'args' if it is not empty."};
      }

      YAML::Node bm_matrix = raw_bm_args["matrix"];
      if (bm_matrix) {
        std::vector<BenchmarkConfig> matrix = create_benchmark_matrix(pmem_directory, bm_args, bm_matrix);
        const std::filesystem::path pmem_data_file = generate_random_file_name(pmem_directory);
        for (BenchmarkConfig& bm : matrix) {
          // Generate unique file for benchmarks that write or reuse existing file for read-only benchmarks.
          if (bm.write_ratio > 0) {
            benchmarks.emplace_back(name, bm);
          } else {
            benchmarks.emplace_back(name, bm, pmem_data_file);
          }
        }
      } else {
        BenchmarkConfig bm_config = BenchmarkConfig::decode(bm_args);
        bm_config.pmem_directory = pmem_directory;
        benchmarks.emplace_back(name, bm_config);
      }
    }
  }
  return benchmarks;
}

std::vector<BinaryBenchmark> BenchmarkFactory::create_parallel_benchmarks(const std::filesystem::path& pmem_directory,
                                                                          std::vector<YAML::Node>& configs) {
  std::vector<BinaryBenchmark> benchmarks{};

  for (YAML::Node& config : configs) {
    for (YAML::iterator it = config.begin(); it != config.end(); ++it) {
      const auto name = it->first.as<std::string>();
      YAML::Node raw_par_bm = it->second;

      // Only consider parallel nodes
      YAML::Node parallel_bm = raw_par_bm["parallel_benchmark"];
      if (parallel_bm.IsDefined()) {
        if (parallel_bm.size() != 2) {
          throw std::invalid_argument{"Number of parallel benchmarks should be two."};
        }

        std::vector<BenchmarkConfig> bm_one_configs{};
        std::vector<BenchmarkConfig> bm_two_configs{};

        YAML::iterator par_it = parallel_bm.begin();
        const auto unique_name_one = par_it->first.as<std::string>();
        YAML::Node raw_bm_args_one = par_it->second;
        YAML::Node bm_args_one = raw_bm_args_one["args"];
        par_it++;
        const auto unique_name_two = par_it->first.as<std::string>();
        YAML::Node raw_bm_args_two = par_it->second;
        YAML::Node bm_args_two = raw_bm_args_two["args"];
        if (!bm_args_one || !bm_args_two) {
          throw std::invalid_argument{"Benchmark config must contain 'args' if it is not empty."};
        }
        YAML::Node bm_matrix_one = raw_bm_args_one["matrix"];
        if (bm_matrix_one) {
          std::vector<BenchmarkConfig> matrix = create_benchmark_matrix(pmem_directory, bm_args_one, bm_matrix_one);
          std::move(matrix.begin(), matrix.end(), std::back_inserter(bm_one_configs));
        } else {
          BenchmarkConfig bm_config = BenchmarkConfig::decode(raw_bm_args_one);
          bm_config.pmem_directory = pmem_directory;
          bm_one_configs.emplace_back(bm_config);
        }
        YAML::Node bm_matrix_two = raw_bm_args_two["matrix"];
        if (bm_matrix_two) {
          std::vector<BenchmarkConfig> matrix = create_benchmark_matrix(pmem_directory, bm_args_two, bm_matrix_two);
          std::move(matrix.begin(), matrix.end(), std::back_inserter(bm_two_configs));
        } else {
          BenchmarkConfig bm_config = BenchmarkConfig::decode(raw_bm_args_two);
          bm_config.pmem_directory = pmem_directory;
          bm_two_configs.emplace_back(bm_config);
        }

        const std::filesystem::path pmem_data_file_one = generate_random_file_name(pmem_directory);
        const std::filesystem::path pmem_data_file_two = generate_random_file_name(pmem_directory);
        // Build cartesian product of both benchmarks
        for (const BenchmarkConfig& config_one : bm_one_configs) {
          for (const BenchmarkConfig& config_two : bm_two_configs) {
            // Generate unique file for benchmarks that write or reuse existing file for read-only benchmarks.
            if (config_one.write_ratio > 0 && config_two.write_ratio > 0) {
              benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two);
            } else if (config_one.write_ratio > 0) {
              benchmarks.emplace_back(name, unique_name_two, unique_name_one, config_two, config_one,
                                      pmem_data_file_two);
            } else if (config_two.write_ratio > 0) {
              benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two,
                                      pmem_data_file_one);
            } else {
              benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two,
                                      pmem_data_file_one, pmem_data_file_two);
            }
          }
        }
      }
    }
  }
  return benchmarks;
}

std::vector<BenchmarkConfig> BenchmarkFactory::create_benchmark_matrix(const std::filesystem::path& pmem_directory,
                                                                       YAML::Node& config_args,
                                                                       YAML::Node& matrix_args) {
  if (!matrix_args.IsMap()) {
    throw std::invalid_argument("'matrix' must be a YAML map.");
  }

  std::vector<BenchmarkConfig> matrix{};
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

      matrix.emplace_back(final_config);
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

std::vector<YAML::Node> BenchmarkFactory::get_config_files(const std::filesystem::path& config_file_path) {
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

  if (config_files.empty()) {
    throw std::invalid_argument{"Benchmark config path " + config_file_path.string() +
                                " must contain at least one config file."};
  }
  std::vector<YAML::Node> yaml_configs{};
  try {
    for (const std::filesystem::path& config_file : config_files) {
      yaml_configs.emplace_back(YAML::LoadFile(config_file));
    }
  } catch (const YAML::ParserException& e1) {
    throw std::runtime_error{"Exception during config parsing: " + e1.msg};
  } catch (const YAML::BadFile& e2) {
    throw std::runtime_error{"Exception during config parsing: " + e2.msg};
  }
  return yaml_configs;
}

}  // namespace perma
