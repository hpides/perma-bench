import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

FPTREE_DATA = {
    "persist": [(32,  9648222), (64, 14227915)], 
    "eADR":    [(32, 10526831), (64, 14970594)], 
}

LBTREE_DATA = {
    "persist": [(32, 28639002), (64, 41241921)], 
    "eADR":    [(32, 33791835), (64, 45549468)], 
}

DASH_DATA =   {
    "persist": [(32, 21667639), (64, 21913650)], 
    "eADR":    [(32, 21868715), (64, 21786738)], 
}

VIPER_DATA =   {
    "persist": [(32, 15747100), (64, 12406100)], 
    "eADR":    [(32, 13960800), (64, 11215000)], 
}

PERSIST_MODE_COLOR = {
    "persist":  '#fd8d3c',
    "eADR": '#fecc5c',
}

PERSIST_MODE_NAME = {
    "persist": 'Regular persist',
    "eADR":  'sfence-only',
}

PERSIST_MODE_HATCH = {
    "persist": '\\\\',
    "eADR": '//',
}

def PERSIST_MODE_BAR(mode):
    return {
        "color": 'white',
        "edgecolor": PERSIST_MODE_COLOR[mode],
        "hatch": PERSIST_MODE_HATCH[mode],
        "lw": 3
    }


def plot_data(system_data, ax,label=False):
    bars = sorted(system_data.keys(), reverse=True)
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    x_pos = range(len(bars))

    for i, system in enumerate(bars):
        data = system_data[system] 
        # print(system, data)
        (x_data, y_data) = zip(*data)
        y_data = [y / MILLION for y in y_data]
        pos = [x + (i * bar_width) for x in x_pos]
        bar = ax.bar(pos, y_data, width=bar_width, **PERSIST_MODE_BAR(system))
        if label:
            bar.set_label(PERSIST_MODE_NAME[system])

    num_xticks = len(x_data)
    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(x_data)
    ax.set_xlabel("\# Threads")


def plot_fptree(ax):
    plot_data(FPTREE_DATA, ax, label=True)

    ax.set_ylim(0, 17)
    ax.set_yticks(range(0, 17, 5))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) FPTree")

def plot_lbtree(ax):
    plot_data(LBTREE_DATA, ax)

    ax.set_ylim(0, 47)
    ax.set_yticks(range(0, 47, 15))
    ax.set_title("b) LB+Tree")

def plot_dash(ax):
    plot_data(DASH_DATA, ax)

    ax.set_ylim(0, 23)
    ax.set_yticks(range(0, 25, 10))
    ax.set_title("c) Dash")

def plot_viper(ax):
    plot_data(VIPER_DATA, ax)

    ax.set_ylim(0, 17)
    ax.set_yticks(range(0, 17, 5))
    ax.set_title("d) Viper")


if __name__ == '__main__':
    skip_dram = True
    _, plot_dir = INIT(sys.argv)

    fig, axes = plt.subplots(1, 4, figsize=(DOUBLE_FIG_WIDTH, 2.7))
    fptree_ax, lbtree_ax, dash_ax, viper_ax = axes

    plot_fptree(fptree_ax)
    plot_lbtree(lbtree_ax)
    plot_dash(dash_ax)
    plot_viper(viper_ax)

    # fig.text(0.5, 0, "\# Threads")

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "eadr_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()