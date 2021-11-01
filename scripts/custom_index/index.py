import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

FF_LOOKUP = {
    "intel-128": [(16, 10989166)],
    "intel-256": [(16, 12108809)]
}

FF_UPDATE = {
    "intel-128": [(16, 4926986)],
    "intel-256": [(16, 5845311)]
}

FF_SCAN = {
    "intel-128": [(16, 2897120)],
    "intel-256": [(16, 3173883)]
}


def plot_data(system_data, ax, offset=0, label=False):
    bars = sorted(system_data.keys(), reverse=True)
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = 2
    x_pos = range(num_xticks)

    first_bar_y = 0
    for i, system in enumerate(bars):
        data = system_data[system]
        if (len(data) == 1):
            xy_data = data[0]
        else:
            xy_data = data[2]

        assert xy_data[0] == 16
        y_data = xy_data[1] / MILLION
        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

        if i == 0:
            first_bar_y = y_data

        if i == 1:
            diff = int((y_data / first_bar_y) * 100)
            ax.text(pos + 0.05, y_data + 0.2, f"{diff}\%", ha="center", size=FS - 8)

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["PerMA", "F+F"])


def plot_lookup(system_data, ax):
    plot_data(system_data, ax, offset=0, label=True)
    plot_data(FF_LOOKUP, ax, offset=1)

    ax.set_ylim(0, 18)
    ax.set_yticks(range(0, 17, 5))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) Lookup")

def plot_update(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(FF_UPDATE, ax, offset=1)

    ax.set_ylim(0, 10)
    ax.set_yticks(range(0, 10, 3))
    ax.set_title("b) Update")

def plot_scan(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(FF_SCAN, ax, offset=1)

    ax.set_ylim(0, 6)
    ax.set_yticks(range(0, 7, 2))
    ax.set_title("c) Scan")


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    lookup_config = {"custom_operations": "r512,r512,r512"}
    lookup_runs = get_runs_from_results(result_path, "primary_index_lookup", lookup_config, skip_dram=skip_dram)
    lookup_data = get_data_from_runs(lookup_runs, "number_threads", "ops_per_second")

    update_config = {"custom_operations": "r512,r512,r512,w64_cache,w64_cache,w64_cache,w64_cache"}
    update_runs = get_runs_from_results(result_path, "primary_index_update", update_config, skip_dram=skip_dram)
    update_data = get_data_from_runs(update_runs, "number_threads", "ops_per_second")

    scan_config = {"custom_operations": "r512,r512,r512,r512,r512,r512,r512"}
    scan_runs = get_runs_from_results(result_path, "primary_index_range_scan", scan_config, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "ops_per_second")

    fig, axes = plt.subplots(1, 3, figsize=DOUBLE_FIG_SIZE)
    lookup_ax, update_ax, scan_ax = axes

    plot_lookup(lookup_data, lookup_ax)
    plot_update(update_data, update_ax)
    plot_scan(scan_data, scan_ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()