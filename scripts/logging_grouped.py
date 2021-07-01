import sys
import matplotlib.pyplot as plt

from common import *

def plot_nocache(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], **LINE(system))
        if 'dram' in system:
            ax.text(16, 30, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])
        if 'hpe' in system:
            ax.text(12.5, 35, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['1', '', '4', '8', '16', '32'])
    ax.set_xlabel("\# of Threads")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 41)
    ax.set_yticks(range(0, 41, 10))

    ax.set_title("a) persist = $\it{NoCache}$")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_none(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        ax.plot(x_data, y_data, **LINE(system))
        if 'dram' in system:
            ax.text(16, 30, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['1', '', '4', '8', '16', '32'])
    ax.set_xlabel("\# of Threads")

    # ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 41)
    ax.set_yticks(range(0, 41, 10))

    ax.set_title("b) persist = $\it{None}$")

    Y_GRID(ax)
    HIDE_BORDERS(ax)



if __name__ == '__main__':
    nocache_filter_config = {
        "access_size": 512,
        "persist_instruction": "nocache",
    }

    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)
    nocache_runs = get_runs_from_results(result_path, "logging", nocache_filter_config, skip_dram=skip_dram)
    nocache_data = get_data_from_runs(nocache_runs, "number_threads", "bandwidth", "write")


    none_filter_config = {
        "access_size": 512,
        "persist_instruction": "none",
    }
    none_runs = get_runs_from_results(result_path, "logging", none_filter_config, skip_dram=skip_dram)
    none_data = get_data_from_runs(none_runs, "number_threads", "bandwidth", "write")

    fig, (nocache_ax, none_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    plot_nocache(nocache_data, nocache_ax)
    plot_none(none_data, none_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "logging_grouped_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
