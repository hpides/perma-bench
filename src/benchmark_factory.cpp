#include "benchmark_factory.hpp"

#include <yaml-cpp/yaml.h>

#include <string>

namespace perma {

std::vector<SingleBenchmark> BenchmarkFactory::create_single_benchmarks(const std::filesystem::path& pmem_directory,
                                                                        std::vector<YAML::Node>& configs,
                                                                        const bool use_dram) {
  std::vector<SingleBenchmark> benchmarks{};

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
        std::vector<BenchmarkConfig> matrix = create_benchmark_matrix(pmem_directory, bm_args, bm_matrix, use_dram);
        const std::filesystem::path pmem_data_file = utils::generate_random_file_name(pmem_directory);
        for (BenchmarkConfig& bm : matrix) {
          // Generate unique file for benchmarks that write or reuse existing file for read-only benchmarks.
          std::vector<std::unique_ptr<BenchmarkResult>> results{};
          results.push_back(std::make_unique<BenchmarkResult>(bm));
          if (bm.contains_write_op()) {
            benchmarks.emplace_back(name, bm, results);
          } else {
            benchmarks.emplace_back(name, bm, results, pmem_data_file);
          }
        }
      } else {
        BenchmarkConfig bm_config = BenchmarkConfig::decode(bm_args);
        bm_config.pmem_directory = pmem_directory;
        bm_config.is_pmem = !use_dram;
        bm_config.is_hybrid = bm_config.dram_operation_ratio > 0.0;
        std::vector<std::unique_ptr<BenchmarkResult>> results{};
        results.push_back(std::make_unique<BenchmarkResult>(bm_config));
        benchmarks.emplace_back(name, bm_config, results);
      }
    }
  }
  return benchmarks;
}

std::vector<ParallelBenchmark> BenchmarkFactory::create_parallel_benchmarks(const std::filesystem::path& pmem_directory,
                                                                            std::vector<YAML::Node>& configs,
                                                                            const bool use_dram) {
  std::vector<ParallelBenchmark> benchmarks{};

  for (YAML::Node& config : configs) {
    for (YAML::iterator it = config.begin(); it != config.end(); ++it) {
      const auto name = it->first.as<std::string>();
      YAML::Node raw_par_bm = it->second;

      // Only consider parallel nodes
      YAML::Node parallel_bm = raw_par_bm["parallel_benchmark"];

      if (!parallel_bm.IsDefined()) {
        continue;
      }

      if (parallel_bm.size() != 2) {
        throw std::invalid_argument{"Number of parallel benchmarks should be two."};
      }

      std::vector<BenchmarkConfig> bm_one_configs{};
      std::vector<BenchmarkConfig> bm_two_configs{};
      std::string unique_name_one;
      std::string unique_name_two;

      YAML::iterator par_it = parallel_bm.begin();
      parse_yaml_node(pmem_directory, bm_one_configs, par_it, unique_name_one, use_dram);
      par_it++;
      parse_yaml_node(pmem_directory, bm_two_configs, par_it, unique_name_two, use_dram);

      const std::filesystem::path pmem_data_file_one = utils::generate_random_file_name(pmem_directory);
      const std::filesystem::path pmem_data_file_two = utils::generate_random_file_name(pmem_directory);
      // Build cartesian product of both benchmarks
      for (const BenchmarkConfig& config_one : bm_one_configs) {
        for (const BenchmarkConfig& config_two : bm_two_configs) {
          // Generate unique file for benchmarks that write or reuse existing file for read-only benchmarks.
          std::vector<std::unique_ptr<BenchmarkResult>> results{};

          // Reorder benchmarks if only the first benchmark is read-only and the second writing
          if (!config_one.contains_write_op() && config_two.contains_write_op()) {
            results.push_back(std::move(std::make_unique<BenchmarkResult>(config_one)));
            results.push_back(std::move(std::make_unique<BenchmarkResult>(config_two)));
          } else {
            results.push_back(std::move(std::make_unique<BenchmarkResult>(config_two)));
            results.push_back(std::move(std::make_unique<BenchmarkResult>(config_one)));
          }

          if (config_one.contains_write_op() && config_two.contains_write_op()) {
            benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two, results);
          } else if (config_one.contains_write_op()) {
            benchmarks.emplace_back(name, unique_name_two, unique_name_one, config_two, config_one, results,
                                    pmem_data_file_two);
          } else if (config_two.contains_write_op()) {
            benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two, results,
                                    pmem_data_file_one);
          } else {
            benchmarks.emplace_back(name, unique_name_one, unique_name_two, config_one, config_two, results,
                                    pmem_data_file_one, pmem_data_file_two);
          }
        }
      }
    }
  }
  return benchmarks;
}

void BenchmarkFactory::parse_yaml_node(const std::filesystem::path& pmem_directory,
                                       std::vector<BenchmarkConfig>& bm_configs, YAML::iterator& par_it,
                                       std::string& unique_name, const bool use_dram) {
  unique_name = par_it->first.as<std::string>();
  YAML::Node raw_bm_args = par_it->second;
  YAML::Node bm_args = raw_bm_args["args"];

  if (!bm_args) {
    throw std::invalid_argument{"Benchmark config must contain 'args' if it is not empty."};
  }
  YAML::Node bm_matrix = raw_bm_args["matrix"];
  if (bm_matrix) {
    std::vector<BenchmarkConfig> matrix = create_benchmark_matrix(pmem_directory, bm_args, bm_matrix, use_dram);
    std::move(matrix.begin(), matrix.end(), std::back_inserter(bm_configs));
  } else {
    BenchmarkConfig bm_config = BenchmarkConfig::decode(bm_args);
    bm_config.pmem_directory = pmem_directory;
    bm_config.is_pmem = !use_dram;
    bm_config.is_hybrid = bm_config.dram_operation_ratio > 0.0;
    bm_configs.emplace_back(bm_config);
  }
}

std::vector<BenchmarkConfig> BenchmarkFactory::create_benchmark_matrix(const std::filesystem::path& pmem_directory,
                                                                       YAML::Node& config_args, YAML::Node& matrix_args,
                                                                       const bool use_dram) {
  if (!matrix_args.IsMap()) {
    throw std::invalid_argument("'matrix' must be a YAML map.");
  }

  std::vector<BenchmarkConfig> matrix{};
  std::set<std::string> matrix_arg_names{};

  std::function<void(const YAML::iterator&, YAML::Node&)> create_matrix = [&](const YAML::iterator& node_iterator,
                                                                              YAML::Node& current_config) {
    YAML::Node current_values = node_iterator->second;
    if (node_iterator == matrix_args.end() || current_values.IsNull()) {
      // End of matrix recursion.
      // We need to copy here to keep the tags clean in the YAML.
      // Otherwise, everything is 'visited' after the first iteration and decoding fails.
      YAML::Node clean_config = YAML::Clone(current_config);

      BenchmarkConfig final_config = BenchmarkConfig::decode(clean_config);
      final_config.pmem_directory = pmem_directory;
      final_config.is_pmem = !use_dram;
      final_config.is_hybrid = final_config.dram_operation_ratio > 0.0;
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
    for (const std::filesystem::path& config_file : std::filesystem::recursive_directory_iterator(config_file_path)) {
      if (config_file.extension() == CONFIG_FILE_EXTENSION) {
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
