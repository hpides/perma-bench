import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

SPEED_COLOR = {
    "2133":      '#fecc5c',
    "2400":      '#fd8d3c',
    "2666":      '#f03b20',
    "b-2933": '#41b6c4', #  '#bd0026',
    "b-3200": '#2c7fb8', # '#bd0026',
}

SPEED_NAME = {
    "2133": 'A-2133',
    "2400": 'A-2400',
    "2666": 'A-2666',
    "b-2933": 'B-2933',
    "b-3200": 'B-3200',
}

SPEED_HATCH = {
    "2133": '\\\\',
    "2400": '//',
    "2666": '\\',
    "b-2933": '/',
    "b-3200": 'x',
}

def SPEED_BAR(system):
    return {
        "color": 'white',
        "edgecolor": SPEED_COLOR[system],
        "hatch": SPEED_HATCH[system],
        "lw": 3
    }

def plot_bm(system_data, ax, offset, label=False):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data = system_data[system]
        y_data = data[-1][1]
        if y_data > MILLION:
            y_data = y_data / MILLION
        # print(system, y_data)
        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **SPEED_BAR(system))
        if label:
            label = SPEED_NAME[system]
            bar.set_label(label)


def plot_raw(scan_data, logging_data, index_data, ax):
    plot_bm(scan_data, ax, 0, label=True)
    plot_bm(logging_data, ax, 1)
    plot_bm(index_data, ax, 2)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 5, 5, 3))
    ax.set_xticklabels(["Seq.\nRead", "Seq.\nWrite", "Rnd.\nRead"])
    ax.set_ylabel("Throughput\n(GB/s)")
    ax.set_ylim(0, 63)
    ax.set_yticks(range(0, 61, 20))
    ax.set_title("a) Raw")


def plot_lat(latency_data, ax):
    plot_bm(latency_data, ax, 0)
    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 5, 5, 1))
    ax.set_xticklabels(["64 Byte\nRead"])
    ax.set_ylabel("Latency (ns)")
    ax.set_ylim(0, 600)
    ax.set_yticks(range(0, 601, 250))
    ax.set_title("b) Latency", loc='center')


def plot_ops(hash_data, tree_data, ax):
    plot_bm(tree_data, ax, 0)
    plot_bm(hash_data, ax, 1)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 5, 5, 2))
    ax.set_xticklabels(["Tree\nLookup", "Update\nHash"])
    ax.set_ylabel("Million Ops/s")
    ax.set_ylim(0, 37)
    ax.set_yticks(range(0, 37, 10))
    ax.set_title("c) Index", loc='center')


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    scan_config = {"access_size": 4096}
    scan_runs = get_runs_from_results(result_path, "sequential_reads", scan_config, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth")

    logging_config = {"access_size": 512, "persist_instruction": "nocache"}
    logging_runs = get_runs_from_results(result_path, "sequential_writes", logging_config, skip_dram=skip_dram)
    logging_data = get_data_from_runs(logging_runs, "number_threads", "bandwidth")

    index_config = {"access_size": 256}
    index_runs = get_runs_from_results(result_path, "random_reads", index_config, skip_dram=skip_dram)
    index_data = get_data_from_runs(index_runs, "number_threads", "bandwidth")

    hash_index_config = {"custom_operations": "rp_512,wp_64_cache_128,wp_64_cache_-128"}
    hash_index_runs = get_runs_from_results(result_path, "hash_index_update", hash_index_config, skip_dram=skip_dram)
    hash_index_data = get_data_from_runs(hash_index_runs, "number_threads", "ops_per_second")

    tree_index_config = {"custom_operations": "rp_512,rp_512,rp_512"}
    tree_index_runs = get_runs_from_results(result_path, "tree_index_lookup", tree_index_config, skip_dram=skip_dram)
    tree_index_data = get_data_from_runs(tree_index_runs, "number_threads", "ops_per_second")

    latency_config = {"custom_operations": "rp_64"}
    latency_runs = get_runs_from_results(result_path, "operation_latency", latency_config, skip_dram=skip_dram)
    latency_data = get_data_from_runs(latency_runs, "number_threads", "latency", "avg")

    fig, axes = plt.subplots(1, 3, figsize=(DOUBLE_FIG_WIDTH, 3), gridspec_kw={'width_ratios': [3, 1, 2]})
    raw_ax, lat_ax, ops_ax = axes

    plot_raw(scan_data, logging_data, index_data, raw_ax)
    plot_lat(latency_data, lat_ax)
    plot_ops(hash_index_data, tree_index_data, ops_ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "speed_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()