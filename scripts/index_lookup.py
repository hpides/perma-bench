import sys
import matplotlib.pyplot as plt

from common import *

def plot_bw(system_data, plot_dir):
    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)

    bar_width = 0.8
    for i, (system, data) in enumerate(sorted(system_data.items())):
        _, y_data = zip(*data)
        ax.bar(i, y_data, color=SYSTEM_COLOR[system], label=system)

    x_ticks = sorted(system_data.keys())
    ax.set_xticks(range(len(x_ticks)))
    ax.set_xticklabels(x_ticks)
    ax.set_ylabel("Bandwidth (GB/s)")

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.13), ncol=5,
               frameon=False, columnspacing=1, handletextpad=0.3)

    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "index_lookup_bw")
    SAVE_PLOT(plot_path)

def plot_lat(system_data_avg, system_data_99, plot_dir):
    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)

    bars = sorted(system_data_avg.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data_avg = system_data_avg[system]
        data_99 = system_data_99[system]
        _, y_data_avg = zip(*data_avg)
        _, y_data_99 = zip(*data_99)
        color = SYSTEM_COLOR[system]
        ax.bar(0 + i * bar_width, y_data_avg, width=bar_width, color=color, label=system)
        ax.bar(1 + i * bar_width, y_data_99, width=bar_width, color=color)

    x_ticks = ['AVG', '99%']
    ax.set_xticks([i + (bar_width / 2) for i in range(len(x_ticks))])
    ax.set_xticklabels(x_ticks)
    ax.set_ylabel("Latency (ns)")

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.13), ncol=5,
               frameon=False, columnspacing=1, handletextpad=0.3)

    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "index_lookup_lat")
    SAVE_PLOT(plot_path)


if __name__ == '__main__':
    filter_config = {
        "access_size": 256,
        "random_distribution": "uniform",
        "number_partitions": 1
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "index_lookup", filter_config)
    bw_data = get_data_from_runs(runs, "access_size", "bandwidth", "read")
    lat_data_avg = get_data_from_runs(runs, "access_size", "duration", "avg")
    lat_data_99 = get_data_from_runs(runs, "access_size", "duration", "percentile_99")

    plot_bw(bw_data, plot_dir)
    plot_lat(lat_data_avg, lat_data_99, plot_dir)

    PRINT_PLOT_PATHS()
