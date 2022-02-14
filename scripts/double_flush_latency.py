import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

OP_NAMES = {
    "rp_64": "Read",
    "rp_64,wp_64_none,wp_64_none": "Write (None)",
    "rp_64,wp_64_cache,wp_64_cache": "Write (Cache)",
    "rp_64,wp_64_cacheinv,wp_64_cacheinv": "Write (CacheInv)",
    "rp_64,wp_64_nocache,wp_64_nocache": "Write (NoCache)"
}


def plot_latency(system_data, ax, label=True):
    filtered_data = {}
    for sys, data in system_data.items():
        f_data = [d for d in data if "256" in d[0]]
        filtered_data[sys] = f_data

    bars = sorted(system_data.keys(), reverse=False)
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(list(system_data.items())[0][1])

    for i, system in enumerate(bars):
        data = system_data[system]
        for j, (_, y_data) in enumerate(data):
            pos = j + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["Read", "+$\it{None}$", "+$\it{Cache}$", "+$\it{CacheInv}$", "+$\it{NoCache}$"])

    ax.set_ylabel("Latency (ns)")  


if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    latency_config = {"number_threads": 32}
    latency_runs = get_runs_from_results(result_path, "double_flush_latency", latency_config, skip_dram=skip_dram)
    latency_data = get_data_from_runs(latency_runs, "custom_operations", "latency", "avg")
    tail_latency_data = get_data_from_runs(latency_runs, "custom_operations", "latency", "percentile_95")

    # latency_fig, axes = plt.subplots(1, 1, figsize=DOUBLE_FIG_SIZE)
    # latency_ax, tail_ax = axes

    latency_fig, axes = plt.subplots(1, 1, figsize=DOUBLE_FIG_SIZE)
    latency_ax = axes

    plot_latency(latency_data, latency_ax)
    # plot_latency(tail_latency_data, tail_ax, False)

    latency_ax.set_ylim(0, 1900)
    latency_ax.set_yticks(range(0, 1901, 300))

    # tail_ax.set_ylim(0, 1)
    # tail_ax.set_yticks(range(0, 1901, 300))

    HATCH_WIDTH()
    FIG_LEGEND(latency_fig)
    for ax in [axes]:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "double_flush_latency")
    SAVE_PLOT(plot_path)

    PRINT_PLOT_PATHS()