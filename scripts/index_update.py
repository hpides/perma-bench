import sys
import matplotlib.pyplot as plt

from common import *

def plot_lat(system_data, ax, title):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(system_data[bars[0]])
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        data = system_data[system]
        x_data, y_data = zip(*data)
        y_data = [y / 1 for y in y_data]
        pos = [x + (i * bar_width) for x in x_pos]
        label = SYSTEM_NAME[system]
        ax.bar(pos, y_data, width=bar_width, label=label,
               **BAR(system))
        # ax.plot(x_data, y_data, label=label, **LINE(system))

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([64, 256, 1024])
    ax.set_xlabel("Access Size in Byte")

    ax.set_ylabel("Latency (ns)")
    ax.set_title(title)

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_bw(system_data_read, system_data_write, ax, title):
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
        # ax.plot(x_data, y_data, label=label, **LINE(system))

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([64, 256, 1024])
    ax.set_xlabel("Access Size in Byte")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_title(title)

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


if __name__ == '__main__':
    filter_config = {
        # "access_size": 64,
        "number_partitions": 1,
        "random_distribution": "uniform",
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "index_update", filter_config, skip_dram=False)
    avg_data = get_data_from_runs(runs, "access_size", "duration", "avg")

    bw_read_data = get_data_from_runs(runs, "access_size", "bandwidth", "read")
    bw_write_data = get_data_from_runs(runs, "access_size", "bandwidth", "write")

    fig, (bw_ax, avg_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_bw(bw_read_data, bw_write_data, bw_ax, "a) Bandwidth")
    plot_lat(avg_data, avg_ax, "b) Latency")

    avg_ax.set_ylim(0, 1400)
    avg_ax.set_yticks(range(0, 1400, 200))
    avg_ax.set_yticklabels(['0', '', '400', '', '800', '', '1200'])

    bw_ax.set_ylim(0, 75)
    bw_ax.set_yticks(range(0, 75, 15))

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "index_update_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
