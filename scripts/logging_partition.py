import sys
import matplotlib.pyplot as plt

from common import *

def plot(system_data, plot_dir):
    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)

    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], lw=3, ms=10,
                color=SYSTEM_COLOR[system], marker=SYSTEM_MARKER[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['64', '', '256', '512', '1024'])
    ax.set_xlabel("Access Size in Byte")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 22)

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.13), ncol=5,
               frameon=False, columnspacing=1, handletextpad=0.3)

    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "logging_partition")
    SAVE_PLOT(plot_path)


if __name__ == '__main__':
    filter_config = {
        "persist_instruction": "nocache",
        "number_partitions": 8,
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "logging_partition", filter_config)
    data = get_data_from_runs(runs, "access_size", "bandwidth", "write")

    plot(data, plot_dir)
    PRINT_PLOT_PATHS()
