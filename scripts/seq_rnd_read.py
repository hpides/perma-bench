import sys
import matplotlib.pyplot as plt
from matplotlib.legend_handler import HandlerTuple

from common import *

def plot_scan(system_data, ax):
    for system, data in sorted(system_data.items()):
        x_data, y_data = zip(*data)
        # print(f'{system}: {data}')

        ax.plot(x_data, y_data, label=SYSTEM_NAME[system], **LINE(system))
        if 'dram' in system:
            ax.text(7, 50, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])
        if 'hpe' in system:
            ax.text(8, 50, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    ax.set_xticks(x_data)
    ax.set_xticklabels(['1', '', '4', '8', '16', '32'])
    ax.set_xlabel("\# of Threads")

    ax.set_ylabel("Throughput (GB/s)")
    ax.set_ylim(0, 65)
    ax.set_yticks(range(0, 65, 10))

    ax.set_title("a) Sequential Reads")


def plot_lookup(system_data, ax):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(system_data[bars[0]])
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        data = system_data[system]
        _, y_data = zip(*data)
        # y_data = y_data[0]
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, label=SYSTEM_NAME[system], **BAR(system))
        if 'dram' in system:
            ax.text(pos[0] + 0.25, 40.5, int(y_data[0]), ha='center', color=SYSTEM_COLOR[system])
            ax.text(pos[1] + 0.25, 40.5, int(y_data[1]), ha='center', color=SYSTEM_COLOR[system])
            ax.text(pos[2], 45.5, int(y_data[2]), ha='center', color=SYSTEM_COLOR[system])

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([64, 256, 1024])
    # ax.set_ylabel("Throughput (GB/s)")
    ax.set_title("b) Random Reads")
    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))
    ax.set_xlabel("Access Size in Byte")



if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    table_scan_filter = {"access_size": 4096}
    scan_runs = get_runs_from_results(result_path, "sequential_reads", table_scan_filter, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth")

    lookup_config = {"number_threads": 16}
    lookup_runs = get_runs_from_results(result_path, "random_reads", lookup_config, skip_dram=skip_dram)
    lookup_data = get_data_from_runs(lookup_runs, "access_size", "bandwidth")


    fig, axes = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    (scan_ax, lookup_ax) = axes

    plot_scan(scan_data, scan_ax)
    plot_lookup(lookup_data, lookup_ax)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    HATCH_WIDTH()

    scan_handles, labels = scan_ax.get_legend_handles_labels()
    lookup_handles, _ = lookup_ax.get_legend_handles_labels()
    legend_handles = zip(scan_handles, lookup_handles)

    fig.legend(legend_handles, labels,
            handler_map={tuple: HandlerTuple(ndivide=2)},
            loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=5,
            frameon=False, columnspacing=0.7, handletextpad=0.3, handlelength=3.2
            #, borderpad=0.1, labelspacing=0.1, handlelength=1.8
    )
    fig.tight_layout()

    plot_path = os.path.join(plot_dir, "seq_rnd_read_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
