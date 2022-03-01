import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

FF_LOOKUP = {
    "apache-128": [(16, 10989166)],
    "apache-256": [(16, 12108809)],
    "apache-512": [(16, 11831716)],
    "barlow-256": [(16, 12255313)],
}

FF_UPDATE = {
    "apache-128": [(16, 4926986)],
    "apache-256": [(16, 5845311)],
    "apache-512": [(16, 5306726)],
    "barlow-256": [(16, 6275601)],
}

FF_SCAN = {
    "apache-128": [(16, 2897120)],
    "apache-256": [(16, 3173883)],
    "apache-512": [(16, 3154291)],
    "barlow-256": [(16, 3665585)],
}


def plot_data(system_data, ax, offset=0, label=False):
    bars = ("apache-128", "apache-256", "apache-512", "barlow-256")
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = 2
    x_pos = range(num_xticks)

    first_bar_y = 0
    for i, system in enumerate(bars):
        data = system_data[system]
        # print(system, data)
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
            y_offset = (first_bar_y * 1.05) - first_bar_y
        

        # diff = int((y_data / first_bar_y) * 100)
        # if 'barlow-256' in system and offset == 0:
        #     ax.text(pos + 0.21, y_data * 1.1 , f"{diff}\%", ha="center", va='top', rotation=90) #, color=SYSTEM_COLOR[system])
        # else:
        #     ax.text(pos + 0.01, y_data + y_offset, f"{diff}\%", ha="center", va='bottom', rotation=90) #, color=SYSTEM_COLOR[system])

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["PerMA", "F+F"])


def plot_lookup(system_data, ax):
    plot_data(system_data, ax, offset=0, label=True)
    plot_data(FF_LOOKUP, ax, offset=1)

    ax.set_ylim(0, 23)
    ax.set_yticks(range(0, 25, 10))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) Lookup")

def plot_update(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(FF_UPDATE, ax, offset=1)

    ax.set_ylim(0, 12)
    ax.set_yticks(range(0, 13, 5))
    ax.set_title("b) Update")

def plot_scan(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(FF_SCAN, ax, offset=1)

    ax.set_ylim(0, 8)
    ax.set_yticks(range(0, 9, 3))
    ax.set_title("c) Scan")


if __name__ == '__main__':
    skip_dram = False
    show_prices = True
    result_path, plot_dir = INIT(sys.argv)

    lookup_config = {"custom_operations": "rp_512,rp_512,rp_512"}
    lookup_runs = get_runs_from_results(result_path, "tree_index_lookup", lookup_config, skip_dram=skip_dram)
    lookup_data = get_data_from_runs(lookup_runs, "number_threads", "ops_per_second")

    update_config = {"custom_operations": "rp_512,rp_512,rp_512,wp_64_cache_320,wp_64_cache_-64,wp_64_cache_-64,wp_64_cache_-64"}
    update_runs = get_runs_from_results(result_path, "tree_index_update", update_config, skip_dram=skip_dram)
    update_data = get_data_from_runs(update_runs, "number_threads", "ops_per_second")

    scan_config = {"custom_operations": "rp_512,rp_512,rp_512,rp_512,rp_512,rp_512,rp_512"}
    scan_runs = get_runs_from_results(result_path, "tree_index_range_scan", scan_config, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "ops_per_second")

    fig, axes = plt.subplots(1, 3, figsize=(DOUBLE_FIG_WIDTH, 2.5))
    lookup_ax, update_ax, scan_ax = axes

    plot_lookup(lookup_data, lookup_ax)
    plot_update(update_data, update_ax)
    plot_scan(scan_data, scan_ax)

    if show_prices:
        print("Lookup prices scaled by 1_000_000", calc_prices(lookup_data, 1_000_000))

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "pmem_tree_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()