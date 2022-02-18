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
        y_data = data[-2]
        assert y_data[0] == 16
        y_data = y_data[1]
        # print(f"sys: {system}, y: {y_data}")
        if y_data > MILLION:
            y_data = y_data / MILLION
        
        if i == 0: base_y = y_data

        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **DIMM_BAR(system))
        if label:
            label = DIMM_NAME[system]
            bar.set_label(label)

        if x_offset != 0:
            if y_data == base_y: diff_str = "1x"
            else: diff_str = f"{(y_data / base_y):0.1f}x"
            ax.text(pos, y_data + x_offset, diff_str, ha='center', 
                    rotation=90, color=DIMM_COLOR[system])


def plot_read(scan_data, index_data, ax):
    x_off=3
    plot_bm(scan_data, ax, 0, x_offset=x_off, label=True)
    plot_bm(index_data, ax, 1, x_offset=x_off)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 2))
    ax.set_xticklabels(["Seq. Read", "Rnd. Read"])
    ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 44)
    ax.set_yticks(range(0, 41, 10))
    ax.set_xlabel("a) Read") #, fontweight='bold')

def plot_write(seq_write_data, rnd_write_data, ax):
    x_off=1.5
    plot_bm(seq_write_data, ax, 0, x_offset=x_off)
    plot_bm(rnd_write_data, ax, 1, x_offset=x_off)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 2))
    ax.set_xticklabels(["Seq. Write", "Rnd. Write"])
    ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 18)
    ax.set_yticks(range(0, 18, 5))
    ax.set_xlabel("b) Write") #, fontweight='bold')


def plot_lat(latency_data, write_latency_data, ax):
    x_off = 80
    plot_bm(latency_data, ax, 0, x_offset=x_off)
    # plot_bm(write_latency_data, ax, 1)
    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 1))
    ax.set_xticklabels(["64 Byte Read"])
    ax.set_ylabel("Latency (ns)")
    ax.set_ylim(0, 850)
    ax.set_yticks(range(0, 900, 200))
    ax.set_xlabel("c) Latency") #, fontweight='bold')


def plot_ops(hash_data, tree_data, ax):
    x_off = 2.5
    plot_bm(tree_data, ax, 0, x_offset=x_off)
    plot_bm(hash_data, ax, 1, x_offset=x_off)

    ax.set_xticks(BAR_X_TICKS_POS(0.8 / 4, 4, 2))
    ax.set_xticklabels(["Tree Lookup", "Hash Update"])
    ax.set_ylabel("Million Ops/s")
    ax.set_ylim(0, 32)
    ax.set_yticks(range(0, 31, 10))
    ax.set_xlabel("d) Index") #, fontweight='bold')


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    scan_config = {"access_size": 4096}
    scan_runs = get_runs_from_results(result_path, "sequential_reads", scan_config, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth")

    logging_config = {"access_size": 512, "persist_instruction": "nocache"}
    logging_runs = get_runs_from_results(result_path, "sequential_writes", logging_config, skip_dram=skip_dram)
    logging_data = get_data_from_runs(logging_runs, "number_threads", "bandwidth")

    rnd_write_config = {"access_size": 64, "persist_instruction": "nocache"}
    rnd_write_runs = get_runs_from_results(result_path, "random_writes", rnd_write_config, skip_dram=skip_dram)
    rnd_write_data = get_data_from_runs(rnd_write_runs, "number_threads", "bandwidth")

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

    write_latency_config = {"custom_operations": "rp_64,wp_64_cache"}
    write_latency_runs = get_runs_from_results(result_path, "operation_latency", write_latency_config, skip_dram=skip_dram)
    write_latency_data = get_data_from_runs(write_latency_runs, "number_threads", "latency", "avg")

    fig, axes = plt.subplots(1, 4, figsize=(2 * DOUBLE_FIG_WIDTH, DOUBLE_FIG_HEIGHT), gridspec_kw={'width_ratios': [2, 2, 1, 2]})
    read_ax, write_ax, lat_ax, ops_ax = axes

    plot_read(scan_data, index_data, read_ax)
    plot_write(logging_data, rnd_write_data, write_ax)
    plot_lat(latency_data, write_latency_data, lat_ax)
    plot_ops(hash_index_data, tree_index_data, ops_ax)

    HATCH_WIDTH()
    # FIG_LEGEND(fig)
    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.05), ncol=4,
               frameon=False, columnspacing=1, handletextpad=0.3)
    fig.tight_layout(w_pad=0)
    # fig.subplots_adjust(wspace=0.6)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "dimm_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()