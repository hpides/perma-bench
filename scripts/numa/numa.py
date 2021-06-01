import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from matplotlib.lines import Line2D
from common import *


NUMA_PATTERN = {
    'far':  'Far',
    'near':  'Near',
    'none':  'None',
}

NUMA_LINE_STYLE = {
    'near': '-',
    'far': ':'
}


def NUMA_LINE(numa_pattern, system):
    sys_line = LINE(system)
    sys_line['ls'] = NUMA_LINE_STYLE[numa_pattern]
    return sys_line


def plot_read(system_data, ax):
    for numa_pattern, systems in system_data.items():
        for system, data in systems.items():
            x_data, y_data = zip(*data[:-1])
            ax.plot(x_data, y_data,
            label=f"{SYSTEM_NAME[system]}-{NUMA_PATTERN[numa_pattern]}",
            **NUMA_LINE(numa_pattern, system))

    ax.plot(1, 0, color='white')

    ax.set_xticks([0, 0.2, 0.4, 0.6, 0.8, 1])
    ax.set_xticklabels(['0', '.2', '.4', '.6', '.8', '1'])
    ax.set_xlabel("Write Ratio")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 17)
    ax.set_yticks(range(0, 16, 3))

    ax.set_title("a) Read")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_write(system_data, ax):
    for numa_pattern, systems in system_data.items():
        for system, data in systems.items():
            x_data, y_data = zip(*data[1:])
            ax.plot(x_data, y_data, **NUMA_LINE(numa_pattern, system))
    ax.plot(0, 0, color='white')

    ax.set_xticks([0, 0.2, 0.4, 0.6, 0.8, 1])
    ax.set_xticklabels(['0', '.2', '.4', '.6', '.8', '1'])
    ax.set_xlabel("Write Ratio")

    ax.set_ylim(0, 3.5)
    ax.set_yticks(range(0, 4, 1))

    ax.set_title("b) Write")
    ax.set_ylabel("Bandwidth (GB/s)")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_lat(system_data, ax):
    for numa_pattern, systems in system_data.items():
        for system, data in systems.items():
            x_data, y_data = zip(*data)
            ax.plot(x_data, y_data, **NUMA_LINE(numa_pattern, system))
    # ax.plot(0, 0, color='white')

    ax.set_xticks([0, 0.2, 0.4, 0.6, 0.8, 1])
    ax.set_xticklabels(['0', '.2', '.4', '.6', '.8', '1'])
    ax.set_xlabel("Write Ratio")

    ax.set_ylim(0, 750)
    ax.set_yticks(range(0, 800, 100))

    ax.set_title("c) Avg. Latency")
    ax.set_ylabel("Latency (ns)")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


if __name__ == '__main__':
    filter_config_far = {
        "numa_pattern": "far"
    }
    filter_config_near = {
        "numa_pattern": "near"
    }

    result_path, plot_dir = INIT(sys.argv)

    runs_far = get_runs_from_results(result_path, "index_update", filter_config_far)
    runs_near = get_runs_from_results(result_path, "index_update", filter_config_near)

    bw_data_write = {}
    bw_data_read = {}
    lat_data = {}

    bw_data_write['far'] = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "write")
    bw_data_write['near'] = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "write")

    bw_data_read['far'] = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "read")
    bw_data_read['near'] = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "read")

    lat_data['near'] = get_data_from_runs(runs_near, "write_ratio", "duration", "avg")
    lat_data['far'] = get_data_from_runs(runs_far, "write_ratio", "duration", "avg")

    fig, (read_ax, write_ax, lat_ax) = plt.subplots(1, 3, figsize=DOUBLE_FIG_SIZE, constrained_layout=True)

    plot_read(bw_data_read, read_ax)
    plot_write(bw_data_write, write_ax)
    plot_lat(lat_data, lat_ax)

    legend_elements = [
        Line2D([], [], color='grey', lw=4, ls=':', label='Far'),
        Line2D([], [], color='grey', lw=4, ls='-', label='Near'),
        Line2D([], [], **LINE('intel-128'), ls='None', label=SYSTEM_NAME['intel-128']),
        Line2D([], [], **LINE('intel-256'), ls='None', label=SYSTEM_NAME['intel-256']),

    ]
    fig.legend(handles=legend_elements, loc='upper center',
                bbox_to_anchor=(0.5, 1.17), ncol=5, frameon=False,
                handletextpad=0.2, labelspacing=0.1, handlelength=1)

    # fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.17), ncol=5,
    #            frameon=False, columnspacing=0.6, handletextpad=0.2,
    #            borderpad=0.1, labelspacing=0.1, handlelength=1.4)
    # fig.tight_layout()
    # fig.subplots_adjust(wspace=0.3)

    plot_path = os.path.join(plot_dir, "numa_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
