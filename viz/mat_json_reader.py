import json

from collections import defaultdict


class MatrixJsonReader:
    """
        This class reads a non-raw result JSON and stores each measured value once as a key in a dictionary. Thus, each
        key holds a list for each benchmark in the JSON.
    """

    def __init__(self, path):
        with open(path) as f:
            json_obj = json.load(f)

        self.results = defaultdict(list)
        benchmarks = list()

        # set main fields of each benchmark
        for bm in json_obj:
            self.results["bm_name"].append(bm["bm_name"])

            # add empty list to matrix_args if benchmark has none
            if "matrix_args" not in bm:
                self.results["matrix_args"].append(list())
            else:
                self.results["matrix_args"].append(bm["matrix_args"])

            benchmarks.append(bm["benchmarks"])

        # set subfields of bandwidth, config and duration field
        self.set_benchmark_subfields(benchmarks)

        # set arg lists
        self.continuous_args = ["total_memory_range", "access_size", "write_ratio", "read_ratio", "pause_frequency",
                                "number_partitions", "number_threads", "pause_length_micros", "number_operations",
                                "zipf_alpha"]
        self.categorical_args = ["exec_mode", "data_instruction", "persist_instruction", "random_distribution"]

        # set labels of matrix arguments
        self.arg_labels = dict()
        self.set_arg_labels()

    """ 
        setter:
    """

    def set_benchmark_subfields(self, benchmarks):
        for i, bm in enumerate(benchmarks):
            for bm_block in bm:
                for k in bm_block.items():

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
        for arg in self.continuous_args + self.categorical_args:
            arg_label = arg.replace("_", " ").title()

            if arg in ["total_memory_range", "access_size", "pause_frequency"]:
                arg_label += " (B)"
            elif arg == "pause_length_micros":
                arg_label += r" ($\mu$s)"

            self.arg_labels[arg] = arg_label

    """ 
        getter:
    """

    def get_result(self, name, bm_idx):
        return self.results[name][bm_idx]

    def get_bm_names(self):
        return self.results["bm_name"]

    def get_categorical_args(self):
        return self.categorical_args

    def get_arg_label(self, arg):
        return self.arg_labels[arg]
