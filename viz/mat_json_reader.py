import json
from collections import defaultdict


class MatrixJsonReader:
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

        # set subfields of bandwidth, config and duration field
        self.set_benchmark_subfields(benchmarks)

        # set arg lists
        self.continuous_args = ["total_memory_range", "access_size", "write_ratio", "read_ratio", "pause_frequency",
                             "number_partitions", "number_threads", "pause_length_micros", "number_operations",
                             "zipf_alpha"]
        self.categorical_args = ["exec_mode", "data_instruction", "persist_instruction", "random_distribution"]

        # set labels of matrix arguments
        self.arg_labels = list()
        self.set_arg_labels()

    """ 
        setter:
    """

    def set_benchmark_subfields(self, benchmarks):
        for i in range(len(benchmarks)):
            for j in range(len(benchmarks[i])):
                for k in benchmarks[i][j].items():

                    # create separate entries for bandwidth operations and values
                    if k[0] == "bandwidth":
                        bandwidth_op = list(k[1].keys())[0]
                        bandwidth_value = list(k[1].values())[0]
                        if len(self.results["bandwidth_ops"]) <= i:
                            self.results["bandwidth_ops"].append(list())
                            self.results["bandwidth_values"].append(list())
                        self.results["bandwidth_ops"][i].append(bandwidth_op)
                        self.results["bandwidth_values"][i].append(bandwidth_value)

                    # create entry for each key in config or duration field
                    else:
                        for l in k[1].items():
                            if len(self.results[l[0]]) <= i:
                                self.results[l[0]].append(list())
                            self.results[l[0]][i].append(l[1])

    def set_arg_labels(self):
        # TODO: add units and filler words to continuously args for better readability
        self.arg_labels = {arg: arg.replace("_", " ").title() for arg in self.continuous_args + self.categorical_args}

    """ 
        getter:
    """

    def get_result(self, name, bm_idx):
        return self.results[name][bm_idx]

    def get_num_benchmarks(self):
        return len(self.results["bm_name"])

    def get_categorical_args(self):
        return self.categorical_args

    def get_arg_label(self, arg):
        return self.arg_labels[arg]
