import sys
import matplotlib.pyplot as plt

from common import *

def plot_scan(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        # print(f'{system}: {data}')

        # TODO: remove with new runs on intel-128!
        if x_data[-1] == 36:
            y_data = y_data[:-1]
            x_data = x_data[:-1]

        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], **LINE(system))
        if 'dram' in system:
            ax.text(16, 30, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])
        if 'hpe' in system:
            ax.text(8.5, 60, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['1', '', '4', '8', '16', '32'])
    ax.set_xlabel("\# of Threads")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 65)
    ax.set_yticks(range(0, 65, 10))

    ax.set_title("a) Table Scan")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_copy(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, **LINE(system))
        if 'dram' in system:
            ax.text(16, 30, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xlabel("\# of Threads")

    # ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 65)
    ax.set_yticks(range(0, 65, 10))

    ax.set_title("b) Large Persistent Copy")

    Y_GRID(ax)
    HIDE_BORDERS(ax)



if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    table_scan_filter = {
        "access_size": 4096,
    }
    scan_runs = get_runs_from_results(result_path, "table_scan", table_scan_filter, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth", "read")

    copy_filter_config = {
        "access_size": 134217728,
        "persist_instruction": "nocache",
    }
    copy_runs = get_runs_from_results(result_path, "large_persistent_copy", copy_filter_config, skip_dram=skip_dram)
    copy_data = get_data_from_runs(copy_runs, "number_threads", "bandwidth", "write")

    # fig, (scan_ax, copy_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    # plot_scan(scan_data, scan_ax)
    # plot_copy(copy_data, copy_ax)

    # FIG_LEGEND(fig)

    # plot_path = os.path.join(plot_dir, "large_read_write_performance")
    # SAVE_PLOT(plot_path)
    # PRINT_PLOT_PATHS()

    ###### Plot scan only ######
    fig, scan_ax = plt.subplots(1, 1, figsize=(7.5, SINGLE_FIG_HEIGHT))

    plot_scan(scan_data, scan_ax)
    scan_ax.set_title("")
    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=5,
               frameon=False, columnspacing=0.4, handletextpad=0.3)
    fig.tight_layout()
    plot_path = os.path.join(plot_dir, "table_scan_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
