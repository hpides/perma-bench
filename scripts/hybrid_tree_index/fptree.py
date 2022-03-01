from curses import raw
import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

FPTREE_DATA = {
    "a-256": [((32, 128), 11710114), ((64, 128), 12116118), ((64, 64), 12017683), ((64, 256), 12268020), ((16, 64), 11719342)],
    "b-256": [((32, 128), 14038927), ((64, 128), 14055977), ((64, 64), 13838248), ((64, 256), 14212690), ((16, 64), 14370187)],
}

FPTREE_COLOR = {
    (32, 128): '#fecc5c',
    (64, 128): '#fd8d3c',
    (64, 64): '#f03b20',
    (64, 256): '#bd0026',
    (16, 64): '#2c7fb8',
}

FPTREE_NAME = {
    (32, 128): '#fecc5c',
    (64, 128): '#fd8d3c',
    (64, 64): '#f03b20',
    (64, 256): '#bd0026',
    (16, 64): '#2c7fb8',
}

FPTREE_HATCH = {
    (32, 128): '\\\\',
    (64, 128): '//',
    (64, 64): '\\',
    (64, 256): '/',
    (16, 64): 'x',
}

def FPTREE_BAR(system):
    return {
        "color": 'white',
        "edgecolor": FPTREE_COLOR[system],
        "hatch": FPTREE_HATCH[system],
        "lw": 3
    }



def plot_lbtree(system_data, ax,):
    bars = sorted(system_data.keys(), reverse=False)
    print(list(FPTREE_DATA.items())[0][1])
    num_bars = len(list(FPTREE_DATA.items())[0][1])
    bar_width = 0.8 / num_bars

    num_xticks = len(bars)
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        data = FPTREE_DATA[system]
        # node_sizes = (256, 512, 1024, 2048, 'pf-off')
        # data = system_data[system]
        # print(system, data)
        for x, (size, y) in enumerate(data):
            pos = i + (x * bar_width)
            y = y / MILLION 
            print(system, x, y)
            bar = ax.bar(pos, y, width=bar_width, **FPTREE_BAR(size))
            if i == 0:
                bar.set_label(FPTREE_NAME[size])

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels([bar.upper() for bar in bars])

    ax.set_ylim(0, 15)
    ax.set_yticks(range(0, 15, 3))


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    config = {}
    runs = get_runs_from_results(result_path, "hybrid_tree_index_lbtree", config, skip_dram=skip_dram)
    data = get_data_from_runs(runs, "custom_operations", "...")
    data = {
        'a-256': [],
        'b-256': [],
    }

    fig, (raw_ax, fptree_ax) = plt.subplots(1, 2, figsize=(DOUBLE_FIG_WIDTH, 2.5))
    plot_lbtree(data, fptree_ax)

    fptree_ax.set_ylabel("Operations/s")
    raw_ax.set_title("a) Raw ")
    fptree_ax.set_title("b) FPTree")


    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in (fptree_ax, raw_ax):
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "fptree_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()