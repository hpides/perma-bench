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

        clean_bms = []
        for bm in json_obj:
            # Remove all custom operation benchmarks
            # if "ops_per_second" in bm["benchmarks"][0]["results"]:
            #     continue
            
            # Remove all parallel benchmarks
            if len(bm["benchmarks"][0]["results"]) == 2:
                continue

            clean_bms.append(bm)

        # set main fields of each benchmark
        for bm in clean_bms:
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
        self.continuous_args = ["memory_range", "access_size", "number_partitions", "number_threads", "number_operations", "zipf_alpha"]
        self.categorical_args = ["exec_mode", "persist_instruction", "random_distribution", "numa_pattern", "memory_type", "prefault_file", "custom_operations"]

        # set labels of matrix arguments
        self.arg_labels = dict()
        self.set_arg_labels()

    """
        setter:
    """

    def set_benchmark_subfields(self, benchmarks):
        for i, bm in enumerate(benchmarks):
            for bm_block in bm:
                for key, fields in bm_block.items():
                    for l in fields.items():
                        if l[0] == "latency":
                            val = l[1]["avg"]
                        else:    
                            val = l[1]

                        while len(self.results[l[0]]) <= i:
                            self.results[l[0]].append(list())
                        self.results[l[0]][i].append(val)

    def set_arg_labels(self):
        for arg in self.continuous_args + self.categorical_args:
            arg_label = arg.replace("_", " ").title()

            if arg in ["memory_range", "access_size"]:
                arg_label += " (B)"

            self.arg_labels[arg] = arg_label

    """
        getter:
    """

    def get_result(self, name, bm_idx):
        res = self.results[name]
        return res[bm_idx] if bm_idx < len(res) else [] 

    def get_bm_names(self):
        return self.results["bm_name"]

    def get_categorical_args(self):
        return self.categorical_args

    def get_arg_label(self, arg):
        return self.arg_labels[arg]
