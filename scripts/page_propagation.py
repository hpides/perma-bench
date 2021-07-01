import sys
import matplotlib.pyplot as plt

from common import *

def plot_read(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data[:-1])
        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], **LINE(system))
        if 'hpe' in system:
            ax.text(0.37, 40, int(y_data[0]), ha='center', color=SYSTEM_COLOR[system])

    ax.plot(1, 0, color='white')

    ax.set_xticks([0, 0.2, 0.5, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))

    ax.set_title("a) Read")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_write(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data[1:])
        ax.plot(x_data, y_data, **LINE(system))
        if 'hpe' in system:
            ax.text(0.63, 40, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.plot(0, 0, color='white')

    ax.set_xticks([0, 0.2, 0.5, 0.8, 1])
    ax.set_xlabel("Write Ratio")

    # ax.set_ylabel("writeency (ms)")
    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))

    ax.set_title("b) Write")

    Y_GRID(ax)
    HIDE_BORDERS(ax)


if __name__ == '__main__':
    filter_config = {
        "access_size": 512,
        "access_size": 4096,
    }

    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)
    bms = get_bms(result_path, "page_propagation", skip_dram=skip_dram)

    runs = defaultdict(list)
    for name, res_runs in bms.items():
        for run in res_runs["benchmarks"]:
            rc = run["config"]
            matches = (rc["access_size"] == 4096
                        and rc['number_threads'] == 16)
                        # and rc['write_ratio'] > 0 and rc['write_ratio'] < 1.0)
            if matches:
                runs[name].append(run)

    read_data = get_data_from_runs(runs, "write_ratio", "bandwidth", "read")
    write_data = get_data_from_runs(runs, "write_ratio", "bandwidth", "write")

    fig, (read_ax, write_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)

    plot_read(read_data, read_ax)
    plot_write(write_data, write_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "page_propagation_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
