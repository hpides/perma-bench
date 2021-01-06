import json
from collections import defaultdict

benchmark_configs = defaultdict(dict)


def get_raw_configs(bm_name):
    return benchmark_configs[bm_name]


class RawJsonReader:
    def __init__(self, path):
        with open(path) as f:
            json_obj = json.load(f)

        # set main fields of raw json
        self.bm_name = json_obj["benchmark_name"]
        self.configs = json_obj["config"]
        self.results = json_obj["results"]

        # set dictionary mapping benchmark names to its configs
        benchmark_configs[self.bm_name] = self.configs

    def get_benchmark_name(self):
        return self.bm_name

    """ 
        getter for general results:
    """

    def get_start_timestamps(self):
        return [res["start_timestamp"] for res in self.results]

    def get_end_timestamps(self):
        return [res["end_timestamp"] for res in self.results]

    def get_timestamps(self):
        timestamps = list()
        for s, e in zip(self.get_start_timestamps(), self.get_end_timestamps()):
            timestamps.extend((s, e))
        return timestamps

    def get_types(self):
        return [res["type"] for res in self.results]

    def get_thread_ids(self):
        return [res["thread_id"] for res in self.results]

    """ 
        getter for write and read results:
    """

    def get_bandwidths(self):
        return [res["bandwidth"] for res in self.results if res["type"] == "read" or res["type"] == "write"]

    def get_data_sizes(self):
        return [res["data_size"] for res in self.results if res["type"] == "read" or res["type"] == "write"]

    def get_latencies(self):
        return [res["latency"] for res in self.results if res["type"] == "read" or res["type"] == "write"]

    """ 
        getter for pause results:
    """

    def get_lengths(self):
        return [res["length"] for res in self.results]

    """ 
        further helper functions:
    """

    def is_multithreaded(self):
        return not self.get_thread_ids()[:-1] == self.get_thread_ids()[1:]

    def contains_pauses(self):
        return True if "pause" in self.get_types() else False
