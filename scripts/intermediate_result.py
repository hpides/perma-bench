import sys
import matplotlib.pyplot as plt

from common import *

def plot_bw(system_data, ax):
    num_bars = len(system_data)
    bar_width = 0.8 / num_bars
    x_pos = range(3)

    for i, (system, data) in enumerate(sorted(system_data.items())):
        x_data, y_data = zip(*data)
        color = SYSTEM_COLOR[system]
        hatch = SYSTEM_HATCH[system]
        bar = BAR(system)
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, **bar, label=SYSTEM_NAME[system])
        if 'dram' in system:
            ax.text(pos[-1], 16.5, int(y_data[-1]), ha='center', color=SYSTEM_COLOR[system])

    x_ticks = ['Cache', 'NoCache', 'None']
    assert len(x_ticks) == len(x_data)
    assert all([x.lower() == custom_x.lower() for (x, custom_x) in zip(x_ticks, x_data)])
    x_ticks = [f"$\it{x}$" for x in x_ticks]

    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)
    ax.set_ylabel("Bandwidth (GB/s)")

    ax.set_ylim(0, 16)
    ax.set_yticks(range(0, 16, 3))

    ax.set_title("a) Bandwidth")

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


def plot_lat(system_data, ax):
    num_bars = len(system_data)
    bar_width = 0.8 / num_bars

    x_pos = range(3)
    for i, (system, data) in enumerate(sorted(system_data.items())):
        x_data, y_data = zip(*data)
        color = SYSTEM_COLOR[system]
        hatch = SYSTEM_HATCH[system]
        bar = BAR(system)
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, **bar)

    x_ticks = ['Cache', 'NoCache', 'None']
    assert len(x_ticks) == len(x_data)
    assert all([x.lower() == custom_x.lower() for (x, custom_x) in zip(x_ticks, x_data)])
    x_ticks = [f"$\it{x}$" for x in x_ticks]

    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)

    ax.set_ylim(0, 900)
    ax.set_yticks(range(0, 1000, 150))

    ax.set_ylabel("Latency (ns)")
    ax.set_title("b) Latency")

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)


if __name__ == '__main__':
    filter_config = {
        "number_threads": 32,
        "total_memory_range": 1073741824
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "intermediate_result", filter_config)
    bw_data = get_data_from_runs(runs, "persist_instruction", "bandwidth", "write")

    lat_data = get_data_from_runs(runs, "persist_instruction", "duration", "avg")
    # lat_data = get_data_from_runs(runs, "access_size", "duration", "percentile_99")


    fig, (bw_ax, lat_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_bw(bw_data, bw_ax)
    plot_lat(lat_data, lat_ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "intermediate_result_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()