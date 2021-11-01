import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *


def plot_aggregation(system_data, ax):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars
    num_xticks = 3

    # Filter out thread count == 4
    system_data = {sys: [(thread, y) for thread, y in data if thread != 4] for sys, data in system_data.items()}

    for i, system in enumerate(bars):
        data = system_data[system]
        for j, (num_threads, y_data) in enumerate(data):
            y_data = y_data / MILLION
            pos = j + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        bar.set_label(SYSTEM_NAME[system])

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["1", "8", "16"])

    ax.set_ylim(0, 35)
    ax.set_yticks(range(0, 35, 10))

    ax.set_ylabel("Million Ops/s")
    ax.set_xlabel("\# of Threads")


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    aggregation_config = {"custom_operations": "r256,w128_none"}
    aggregation_runs = get_runs_from_results(result_path, "aggregation", aggregation_config, skip_dram=skip_dram)
    aggregation_data = get_data_from_runs(aggregation_runs, "number_threads", "ops_per_second")

    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)
    plot_aggregation(aggregation_data, ax)

    HATCH_WIDTH()

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=3,
               frameon=False, columnspacing=1, handletextpad=0.3)
    fig.tight_layout()

    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_aggregation")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()