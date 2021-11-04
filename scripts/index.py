import sys
import matplotlib.pyplot as plt

from common import *

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
        if 'hpe' in system:
            ax.text(pos[1] + 0.25, 40.5, int(y_data[1]), ha='center', color=SYSTEM_COLOR[system])
            ax.text(pos[2], 45.5, int(y_data[2]), ha='center', color=SYSTEM_COLOR[system])

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([64, 256, 1024])
    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_title("a) Index Lookup")


def plot_update(system_data_read, system_data_write, ax):
    bars = sorted(system_data_read.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(system_data_read[bars[0]])
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        read_data = system_data_read[system]
        write_data = system_data_write[system]
        xr_data, yr_data = zip(*read_data)
        xw_data, yw_data = zip(*write_data)
        y_data = [yr + yw for yr, yw in zip(yr_data, yw_data)]
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if 'hpe' in system:
            ax.text(pos[1] + 0.25, 40.5, int(y_data[1]), ha='center', color=SYSTEM_COLOR[system])
            ax.text(pos[2], 45.5, int(y_data[2]), ha='center', color=SYSTEM_COLOR[system])

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([64, 256, 1024])
    ax.set_title("b) Index Update")
    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    lookup_config = {"random_distribution": "uniform", "number_partitions": 1}
    lookup_runs = get_runs_from_results(result_path, "index_lookup", lookup_config, skip_dram=skip_dram)
    lookup_data = get_data_from_runs(lookup_runs, "access_size", "bandwidth", "read")

    update_config = {"number_partitions": 1, "random_distribution": "uniform"}
    update_runs = get_runs_from_results(result_path, "index_update", update_config, skip_dram=skip_dram)
    update_read_data = get_data_from_runs(update_runs, "access_size", "bandwidth", "read")
    update_write_data = get_data_from_runs(update_runs, "access_size", "bandwidth", "write")


    fig, axes = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    lookup_ax, update_ax = axes

    plot_lookup(lookup_data, lookup_ax)
    plot_update(update_read_data, update_write_data, update_ax)

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)
        ax.set_ylim(0, 45)
        ax.set_yticks(range(0, 45, 10))
        ax.set_xlabel("Access Size in Byte")

    HATCH_WIDTH()
    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "index_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()