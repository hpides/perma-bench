import json

from collections import defaultdict


class MatrixJsonUtils:
    def __init__(self, path):
        with open(path) as f:
            json_obj = json.load(f)

        self.results = defaultdict(list)
        benchmarks = list()

        # set main fields of each benchmark
        for bm in range(len(json_obj)):
            self.results["bm_name"].append(json_obj[bm]["bm_name"])
            self.results["matrix_args"].append(json_obj[bm]["matrix_args"])
            benchmarks.append(json_obj[bm]["benchmarks"])

        # set subfields of bandwidth, config and duration
        self.set_benchmark_subfields(benchmarks)

        # set arg lists
        self.continuous_args = ["total_memory_range", "access_size", "write_ratio", "read_ratio", "pause_frequency",
                             "number_partitions", "number_threads", "pause_length_micros", "number_operations",
                             "zipf_alpha"]
        self.categorical_args = ["exec_mode", "data_instruction", "persist_instruction", "random_distribution"]

        # set labels of matrix arguments
        self.arg_labels = list()
        self.set_arg_labels()

        # TODO: create list for x-tick labels of enum args

    """ 
        setter:
    """

    def set_benchmark_subfields(self, benchmarks):
        # iterate each benchmark
        for i in range(len(benchmarks)):
            self.results["num_results_per_bm"].append(len(benchmarks[i]))

            # iterate each block with bandwidth, config and duration field
            for j in range(len(benchmarks[i])):

                # iterate bandwidth, config and duration field separately
                for k in benchmarks[i][j].items():

                    # create separate entries for bandwidth operations and values
                    # TODO: is this exception necessary?
                    if k[0] == "bandwidth":
                        bandwidth_op = list(k[1].keys())[0]
                        bandwidth_value = list(k[1].values())[0]
                        if len(self.results["bandwidth_ops"]) > i:
                            self.results["bandwidth_ops"][i].append(bandwidth_op)
                            self.results["bandwidth_values"][i].append(bandwidth_value)
                        else:
                            self.results["bandwidth_ops"].append([bandwidth_op])
                            self.results["bandwidth_values"].append([bandwidth_value])

                    # create entry for each key in config or duration field
                    else:
                        for l in k[1].items():
                            if len(self.results[l[0]]) > i:
                                self.results[l[0]][i].append(l[1])
                            else:
                                # TODO: add exception for l[0] == data_instruction?
                                self.results[l[0]].append([l[1]])

    def set_arg_labels(self):
        # TODO: add units and fill words to some of the continuously args
        self.arg_labels = {arg: arg.replace("_", " ").title() for arg in self.continuous_args + self.categorical_args}

    """ 
        getter:
    """

    def get_num_benchmarks(self):
        return len(self.results["bm_name"])

    def get_continuous_args(self):
        return self.continuous_args

    def get_categorical_args(self):
        return self.categorical_args

    def get_arg_label(self, arg):
        return self.arg_labels[arg]
