from common import *

NUMA_COLOR = {
    'far':  '#41b6c4',
    'near':  '#378d54'
}

NUMA_MARKER = {
    'far':  '^',
    'near':  's'
}

NUMA_PATTERN = {
    'far':  'Far',
    'near':  'Near'
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

    ax.set_xticks([0, 0.2, 0.5, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 12)
    ax.set_yticks(range(0, 15, 4))

    ax.set_title("a) Read")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_write(system_data, ax):
    for numa_pattern, data in system_data.items():
        x_data, y_data = zip(*data[1:])
        ax.plot(x_data, y_data, **NUMA_LINE(numa_pattern))
    ax.plot(0, 0, color='white')

    ax.set_xticks([0, 0.2, 0.5, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    ax.set_ylim(0, 3)
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

    result_path, plot_dir = INIT(sys.argv)

    runs_far = get_runs_from_results(result_path, "index_update", filter_config_far)
    runs_near = get_runs_from_results(result_path, "index_update", filter_config_near)

    bw_data_write_far = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "write")
    bw_data_write_near = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "write")
    bw_data_read_far = get_data_from_runs(runs_far, "write_ratio", "bandwidth", "read")
    bw_data_read_near = get_data_from_runs(runs_near, "write_ratio", "bandwidth", "read")

    bw_data_write = {}
    bw_data_read = {}
    bw_data_write['near'] = list(bw_data_write_near.values())[0]
    bw_data_read['near'] = list(bw_data_read_near.values())[0]
    bw_data_write['far'] = list(bw_data_write_far.values())[0]
    bw_data_read['far'] = list(bw_data_read_far.values())[0]

    fig, (read_ax, write_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    plot_read(bw_data_read, read_ax)
    plot_write(bw_data_write, write_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "numa_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
