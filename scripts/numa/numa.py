import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

NUMA_COLOR = {
    'far':  '#253494',
    'near':  '#41b6c4',
    'none': '#378d54'
}

NUMA_MARKER = {
    'far':  '^',
    'near':  's',
    'none':  'o',
}

NUMA_PATTERN = {
    'far':  'Far',
    'near':  'Near',
    'none':  'None',
}


def NUMA_LINE(numa_pattern):
    return {
        "lw": 4,
        "ms": 10,
        "color": NUMA_COLOR[numa_pattern],
        "marker": NUMA_MARKER[numa_pattern],
        "markeredgewidth": 1,
        "markeredgecolor": 'black',
    }


def plot_read(system_data, ax):
    for numa_pattern, data in system_data.items():
        x_data, y_data = zip(*data[:-1])
        ax.plot(x_data, y_data, label=NUMA_PATTERN[numa_pattern], **NUMA_LINE(numa_pattern))

    ax.plot(1, 0, color='white')

    ax.set_xticks([0, 0.2, 0.4, 0.6, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 21)
    ax.set_yticks(range(0, 18, 6))

    ax.set_title("a) Read")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_write(system_data, ax):
    for numa_pattern, data in system_data.items():
        x_data, y_data = zip(*data[1:])
        ax.plot(x_data, y_data, **NUMA_LINE(numa_pattern))
    ax.plot(0, 0, color='white')

    ax.set_xticks([0, 0.2, 0.4, 0.6, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    ax.set_ylim(0, 3.5)
    ax.set_yticks(range(0, 4, 1))

    ax.set_title("b) Write")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


if __name__ == '__main__':
    filter_config_far = {
        "numa_pattern": "far"
    }
    filter_config_near = {
        "numa_pattern": "near"
    }
    filter_config_none = {
        "numa_pattern": "none"
    }

    result_path, plot_dir = INIT(sys.argv)

    runs_far = get_runs_from_results(result_path, "index_update", filter_config_far)
    runs_near = get_runs_from_results(result_path, "index_update", filter_config_near)
    runs_none = get_runs_from_results(result_path, "index_update", filter_config_none)

    bw_data_write_far = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "write")
    bw_data_write_near = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "write")
    bw_data_write_none = get_data_from_runs(runs_none, "write_ratio", "bandwidth", "write")
    bw_data_read_far = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "read")
    bw_data_read_near = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "read")
    bw_data_read_none = get_data_from_runs(runs_none, "write_ratio", "bandwidth", "read")

    bw_data_write = {}
    bw_data_read = {}
    bw_data_none = {}

    bw_data_write['far'] = list(bw_data_write_far.values())[0]
    bw_data_read['far'] = list(bw_data_read_far.values())[0]
    bw_data_write['near'] = list(bw_data_write_near.values())[0]
    bw_data_read['near'] = list(bw_data_read_near.values())[0]
    bw_data_write['none'] = list(bw_data_write_none.values())[0]
    bw_data_read['none'] = list(bw_data_read_none.values())[0]

    fig, (read_ax, write_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    plot_read(bw_data_read, read_ax)
    plot_write(bw_data_write, write_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "numa_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
