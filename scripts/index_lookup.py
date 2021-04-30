import sys
import matplotlib.pyplot as plt

from common import *

def plot_bw(system_data, ax):
    bar_width = 0.8
    for i, (system, data) in enumerate(sorted(system_data.items())):
        _, y_data = zip(*data)
        y_data = y_data[0]
        ax.bar(i, y_data, width=bar_width,
               label=SYSTEM_NAME[system], **BAR(system))
        if 'dram' in system:
            ax.text(i, 46, int(y_data), ha='center',
                    bbox=dict(facecolor='white', edgecolor='none', pad=0))

    ax.set_xticks([])
    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 50)
    ax.set_yticks(range(0,51, 10))

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_lat(system_data_avg, system_data_99, ax):
    bars = sorted(system_data_avg.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data_avg = system_data_avg[system]
        data_99 = system_data_99[system]
        _, y_data_avg = zip(*data_avg)
        _, y_data_99 = zip(*data_99)
        color = SYSTEM_COLOR[system]
        hatch = SYSTEM_HATCH[system]
        bar = BAR(system)
        ax.bar(0 + i * bar_width, y_data_avg, width=bar_width, **bar)
        ax.bar(1 + i * bar_width, y_data_99,  width=bar_width, **bar)

    x_ticks = ['AVG', '99%']
    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)
    ax.set_ylabel("Latency (ns)")

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


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


    fig, (bw_ax, lat_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_bw(bw_data, bw_ax)
    plot_lat(lat_data_avg, lat_data_99, lat_ax)

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=5,
               frameon=False, columnspacing=1, handletextpad=0.3)
    fig.tight_layout()

    plot_path = os.path.join(plot_dir, "index_lookup_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
