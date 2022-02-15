import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

OP_NAMES = {
    "rp_256": "Read",
    "rp_256,wp_256_none": "Write (None)",
    "rp_256,wp_256_cache": "Write (Cache)",
    "rp_256,wp_256_cacheinv": "Write (CacheInv)",
    "rp_256,wp_256_nocache": "Write (NoCache)"
}


def plot_data(system_data, ax, label=False):
    bars = sorted(system_data.keys(), reverse=False)
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(list(system_data.items())[0][1])
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        data = system_data[system]
        for j, (_, y_data) in enumerate(data):
            pos = j + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))

        if label:
            bar.set_label(SYSTEM_NAME[system])

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))


def plot_lookup(system_data, ax, label=True):
    filtered_data = {}
    for sys, data in system_data.items():
        f_data = [d for d in data if "256" in d[0]]
        filtered_data[sys] = f_data

    plot_data(filtered_data, ax, label=label)
    ax.set_xticklabels(["Read", "$\it{None}$", "$\it{Cache}$", "$\it{CacheInv}$", "$\it{NoCache}$"])

    # ax.set_ylim(0, 18)
    # ax.set_yticks(range(0, 17, 5))

    ax.set_ylabel("Latency (ns)")

def plot_read(system_data, ax):
    plot_data(system_data, ax, label=True)
    ax.set_xticklabels(["1", "16", "32"])

    # ax.set_ylim(0, 12)
    # ax.set_yticks(range(0, 12, 5))
    ax.set_title("a) Read")
    ax.set_ylabel("Latency (ns)")
    ax.set_xlabel("\# of Threads")

def plot_write(system_data, ax):
    plot_data(system_data, ax)
    ax.set_xticklabels(["1", "16", "32"])

    # ax.set_ylim(0, 12)
    # ax.set_yticks(range(0, 12, 5))
    ax.set_title("b) Write")
    ax.set_xlabel("\# of Threads")


if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    latency_config = {"number_threads": 16}
    latency_runs = get_runs_from_results(result_path, "operation_latency", latency_config, skip_dram=skip_dram)
    latency_data = get_data_from_runs(latency_runs, "custom_operations", "latency", "avg")
    tail_latency_data = get_data_from_runs(latency_runs, "custom_operations", "latency", "percentile_99")

    read_config = {"custom_operations": "rp_256"}
    read_runs = get_runs_from_results(result_path, "operation_latency", read_config, skip_dram=skip_dram)
    read_data = get_data_from_runs(read_runs, "number_threads", "latency", "avg")

    write_config = {"custom_operations": "rp_256,wp_256_cache"}
    write_runs = get_runs_from_results(result_path, "operation_latency", write_config, skip_dram=skip_dram)
    write_data = get_data_from_runs(write_runs, "number_threads", "latency", "avg")

    tail_latency_config = {"number_threads": 16}
    tail_latency_runs = get_runs_from_results(result_path, "operation_latency", tail_latency_config, skip_dram=skip_dram)


    ###########################
    # Overall Operations
    ###########################
    # latency_fig, axes = plt.subplots(2, 1, figsize=(DOUBLE_FIG_WIDTH, 1.5 * DOUBLE_FIG_HEIGHT))
    # (latency_ax, tail_latency_ax) = axes
    latency_fig, axes = plt.subplots(1, 1, figsize=DOUBLE_FIG_SIZE)
    latency_ax = axes

    plot_lookup(latency_data, latency_ax)
    # plot_lookup(tail_latency_data, tail_latency_ax, label=False)

    # latency_ax.set_title("a) Average Latency")
    latency_ax.set_ylim(0, 1950)
    latency_ax.set_yticks(range(0, 1900, 300))

    # tail_latency_ax.set_title("b) 99th-Percentile Latency")
    # tail_latency_ax.set_ylim(0, 8500)
    # tail_latency_ax.set_yticks(range(0, 8001, 2000))

    HATCH_WIDTH()
    FIG_LEGEND(latency_fig)
    for ax in [axes]:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_latency")
    SAVE_PLOT(plot_path)

    ###########################
    # Multi-threaded READ/WRITE
    ###########################
    rw_fig, axes = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    read_ax, write_ax = axes

    plot_read(read_data, read_ax)
    plot_write(write_data, write_ax)

    read_ax.set_ylim(0, 1250)
    read_ax.set_yticks(range(0, 1251, 300))
    write_ax.set_ylim(0, 1250)
    write_ax.set_yticks(range(0, 1251, 300))

    HATCH_WIDTH()
    FIG_LEGEND(rw_fig)
    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_rw_latency")
    SAVE_PLOT(plot_path)


    PRINT_PLOT_PATHS()