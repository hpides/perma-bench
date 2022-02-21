import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

DASH_UPDATE = {
    "apache-128": [(16, 10324116)],
    "apache-256": [(16, 13550466)],
    "apache-512": [(16,  9946400)],
    "barlow-256": [(16, 16979811)],
}

DASH_LOOKUP = {
    "apache-128": [(16, 38587320)],
    "apache-256": [(16, 47271039)],
    "apache-512": [(16, 41484534)],
    "barlow-256": [(16, 56744399)],
}


def plot_data(system_data, ax, offset=0, label=False):
    bars = ('apache-256', 'apache-512', 'apache-128', 'barlow-256')
    num_bars = len(bars)
    bar_width = 0.6 / num_bars

    first_bar_y = 0
    for i, system in enumerate(bars):
        data = system_data[system]
        # print(system, data)
        if (len(data) == 1):
            xy_data = data[0]
        else:
            xy_data = data[-2]

        assert xy_data[0] == 16
        y_data = xy_data[1] / MILLION
        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

        if i == 0:
            first_bar_y = y_data
            y_offset = (first_bar_y * 1.05) - first_bar_y
            
        # else:
        diff = int((y_data / first_bar_y) * 100)
        if 'barlow-256' in system and offset == 0:
            ax.text(pos + 0.17, y_data * 1.1 , f"{diff}\%", ha="center", va='top', rotation=90) #, color=SYSTEM_COLOR[system])
        else:
            ax.text(pos + 0.01, y_data + y_offset, f"{diff}\%", ha="center", va='bottom', rotation=90) #, color=SYSTEM_COLOR[system])

    num_xticks = 2
    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["PerMA", "Dash"])


def plot_hash_index_update(system_data, ax):
    plot_data(system_data, ax, offset=0, label=True)
    plot_data(DASH_UPDATE, ax, offset=1)

    ax.set_ylim(0, 43)
    ax.set_yticks(range(0, 44, 10))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) Update")

def plot_hash_index_lookup(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(DASH_LOOKUP, ax, offset=1)

    ax.set_ylim(0, 85)
    ax.set_yticks(range(0, 86, 15))
    ax.set_title("b) Lookup")


if __name__ == '__main__':
    skip_dram = True
    show_prices = True
    result_path, plot_dir = INIT(sys.argv)

    update_config = {"custom_operations": "rp_512,wp_64_cache_128,wp_64_cache_-128"}
    update_runs = get_runs_from_results(result_path, "hash_index_update", update_config, skip_dram=skip_dram)
    update_data = get_data_from_runs(update_runs, "number_threads", "ops_per_second")

    lookup_config = {"custom_operations": "rp_512"}
    lookup_runs = get_runs_from_results(result_path, "hash_index_lookup", lookup_config, skip_dram=skip_dram)
    lookup_data = get_data_from_runs(lookup_runs, "number_threads", "ops_per_second")

    fig, (update_ax, lookup_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_hash_index_update(update_data, update_ax)
    plot_hash_index_lookup(lookup_data, lookup_ax)

    if show_prices:
        print("Update prices scaled by 100_000", calc_prices(update_data, 100_000))

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in (update_ax, lookup_ax):
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "hash_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()