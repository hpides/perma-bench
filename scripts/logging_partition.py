import sys
import matplotlib.pyplot as plt

from common import *

def plot_bw(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], **LINE(system))
        if 'hpe' in system:
            ax.text(512, 25, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['64', '', '256', '512', '1024'])
    ax.set_xlabel("Access Size in Byte")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 28)
    ax.set_yticks(range(0, 28, 5))

    ax.set_title("a) Bandwidth")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_lat(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, **LINE(system))

    ax.set_xticks(x_data)
    ax.set_xticklabels(['64', '', '256', '512', '1024'])
    ax.set_xlabel("Access Size in Byte")

    ax.set_ylabel("Latency (ns)")
    ax.set_ylim(0, 750)
    ax.set_yticks(range(0, 750, 150))

    ax.set_title("b) Latency")

    Y_GRID(ax)
    HIDE_BORDERS(ax)



if __name__ == '__main__':
    skip_dram = False
    filter_config = {
        "persist_instruction": "nocache",
        "number_partitions": 8,
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "logging_partition", filter_config, skip_dram=skip_dram)
    bw_data = get_data_from_runs(runs, "access_size", "bandwidth", "write")
    lat_data = get_data_from_runs(runs, "access_size", "duration", "avg")

    fig, (bw_ax, lat_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    plot_bw(bw_data, bw_ax)
    plot_lat(lat_data, lat_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "logging_partition_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
