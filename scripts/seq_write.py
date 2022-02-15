import sys
import matplotlib.pyplot as plt

from common import *

def plot_threads(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, **LINE(system), label=SYSTEM_NAME[system])
        if 'dram' in system:
            ax.text(13.5, 25, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])
        if 'hpe' in system:
            ax.text(4, 25, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['1', '', '4', '8', '16', '32'])
    ax.set_xlabel("\# of Threads")

    ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 28)
    ax.set_yticks(range(0, 28, 5))

    ax.set_title("a) Thread Count")


def plot_size(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, **LINE(system)) #, label=SYSTEM_NAME[system])
        if 'hpe' in system:
            ax.text(512, 25, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['64', '', '256', '512', '1024'])
    ax.set_xlabel("Access Size in Byte")

    # ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 28)
    ax.set_yticks(range(0, 28, 5))

    ax.set_title("b) Access Size")


if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    threads_config = {"persist_instruction": "nocache", "access_size": 512}
    threads_runs = get_runs_from_results(result_path, "sequential_writes", threads_config, skip_dram=skip_dram)
    threads_data = get_data_from_runs(threads_runs, "number_threads", "bandwidth")

    size_config = {"persist_instruction": "nocache", "number_threads": 8}
    size_runs = get_runs_from_results(result_path, "sequential_writes", size_config, skip_dram=skip_dram)
    size_data = get_data_from_runs(size_runs, "access_size", "bandwidth")

    fig, axes = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    (threads_ax, size_ax) = axes

    plot_threads(threads_data, threads_ax)
    plot_size(size_data, size_ax)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=6,
               frameon=False, columnspacing=1, handletextpad=0.3, handlelength=1.5
               #, borderpad=0.1, labelspacing=0.1, handlelength=1.8
              )
    fig.tight_layout()

    plot_path = os.path.join(plot_dir, "sequential_writes")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
