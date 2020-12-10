import json
from collections import defaultdict

results = dict()
benchmark_configs = defaultdict(dict)


def read_results(path):
    with open(path) as f:
        global results
        results = json.load(f)


def get_benchmark_name():
    return results["benchmark_name"]


def get_config():
    return results["config"]


def set_config_of_benchmark():
    global benchmark_configs
    benchmark_configs[get_benchmark_name()] = get_config()


def init(path):
    read_results(path)
    set_config_of_benchmark()


def get_config_of_benchmark(benchmark_name):
    return benchmark_configs[benchmark_name]


def get_results():
    return results["results"]


def get_bandwidths():
    return [res["bandwidth"] for res in get_results() if res["type"] == "read" or res["type"] == "write"]


def get_data_sizes():
    return [res["data_size"] for res in get_results() if res["type"] == "read" or res["type"] == "write"]


def get_latencies():
    return [res["latency"] for res in get_results() if res["type"] == "read" or res["type"] == "write"]


def get_start_timestamps():
    return [res["start_timestamp"] for res in get_results()]


def get_end_timestamps():
    return [res["end_timestamp"] for res in get_results()]


def get_timestamps():
    timestamps = list()
    for s, e in zip(get_start_timestamps(), get_end_timestamps()):
        timestamps.extend((s, e))
    return timestamps


def get_types():
    return [res["type"] for res in get_results()]


def get_thread_ids():
    return [res["thread_id"] for res in get_results()]


def get_lengths():
    return [res["length"] for res in get_results()]


def is_multithreaded():
    return not get_thread_ids()[:-1] == get_thread_ids()[1:]


def contains_pauses():
    if "pause" in get_types():
        return True
    else:
        return False
