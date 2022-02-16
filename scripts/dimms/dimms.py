import sys
import os
from turtle import pos, position
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

DIMM_COLOR = {
    '1dimms': '#fecc5c', # '#a1dab4',
    '2dimms': '#fd8d3c', # '#378d54',
    '4dimms': '#f03b20', # '#41b6c4',
    '6dimms': '#bd0026', # '#2c7fb8',
}

DIMM_HATCH = {
    '1dimms': '\\\\',
    '2dimms': '//',
    '4dimms': '\\',
    '6dimms': '/',
}

DIMM_NAME = {
    '1dimms': "1 DIMM",
    '2dimms': "2 DIMMs",
    '4dimms': "4 DIMMs",
    '6dimms': "6 DIMMs",
}

def DIMM_BAR(dimm):
    return {
        "color": 'white',
        "edgecolor": DIMM_COLOR[dimm],
        "hatch": DIMM_HATCH[dimm],
        "lw": 3
    }


def plot_bm(system_data, ax, offset, x_offset=2, label=False):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    base_y = 0

    for i, system in enumerate(bars):
        data = system_data[system]
        y_data = data[-1][1]
        if y_data > MILLION:
            y_data = y_data / MILLION
        
        if i == 0: base_y = y_data

        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **DIMM_BAR(system))
        if label:
            label = DIMM_NAME[system]
            bar.set_label(label)

        if x_offset != 0:
            x_x_off = (x_offset * 0.015) if i != 3 else 0
            if y_data == base_y: diff_str = "1x"
            else: diff_str = f"{(y_data / base_y):0.1f}x"
            ax.text(pos - x_x_off, y_data + x_offset, diff_str, ha='center', 
                    rotation=90, color=DIMM_COLOR[system])


def plot_raw(scan_data, logging_data, index_data, ax):
    x_off=3
    plot_bm(scan_data, ax, 0, x_offset=x_off, label=True)
    plot_bm(logging_data, ax, 1, x_offset=x_off)
    plot_bm(index_data, ax, 2, x_offset=x_off)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 3))
    ax.set_xticklabels(["Seq.\nRead", "Seq.\nWrite", "Rnd.\nRead"])
    ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 40)
    ax.set_yticks(range(0, 41, 10))
    ax.set_title("a) Raw")

def plot_ops(hash_data, tree_data, ax):
    plot_bm(tree_data, ax, 0)
    plot_bm(hash_data, ax, 1)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 2))
    ax.set_xticklabels(["Tree\nLookup", "Update\nHash"])
    ax.set_ylabel("Million Ops/s")
    ax.set_ylim(0, 30)
    ax.set_yticks(range(0, 31, 10))
    ax.set_title("b) Index")


def plot_lat(latency_data, ax):
    plot_bm(latency_data, ax, 0, x_offset=0)
    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 1))
    ax.set_xticklabels(["64 Byte\nRead"])
    ax.set_ylabel("Latency (ns)")
    ax.set_ylim(0, 600)
    ax.set_yticks(range(0, 601, 150))
    ax.set_title("c) Latency", loc='right')


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

    fig, axes = plt.subplots(1, 3, figsize=DOUBLE_FIG_SIZE, gridspec_kw={'width_ratios': [3, 2.3, 1]})
    raw_ax, ops_ax, lat_ax = axes

    plot_raw(scan_data, logging_data, index_data, raw_ax)
    plot_ops(hash_index_data, tree_index_data, ops_ax)
    plot_lat(latency_data, lat_ax)

    HATCH_WIDTH()
    # FIG_LEGEND(fig)
    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.05), ncol=4,
               frameon=False, columnspacing=1, handletextpad=0.3)
    fig.tight_layout()
    # fig.subplots_adjust(wspace=0.6)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "dimm_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()