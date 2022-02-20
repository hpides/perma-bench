import sys
import os
from traceback import print_tb
import matplotlib.pyplot as plt
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

PF_MODE_COLOR = {
    "pf-off":  '#fd8d3c',
    "pf-on": '#fecc5c',
}

PF_MODE_NAME = {
    "pf-off": 'Prefetcher Off',
    "pf-on":  'Prefetcher On  ',
}

PF_MODE_HATCH = {
    "pf-off": '\\\\',
    "pf-on": '//',
}

def PF_MODE_BAR(mode):
    
    return {
        "color": 'white',
        "edgecolor": PF_MODE_COLOR[mode],
        "hatch": PF_MODE_HATCH[mode],
        "lw": 3
    }


def plot_read(system_data, ax, title, label=False):
    bars = sorted(system_data.keys(), reverse=True)
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(system_data[bars[0]])
    x_pos = range(num_xticks)

    for i, system in enumerate(bars):
        mode = "pf-off" if "pf-off" in system else "pf-on" 
        data = system_data[system]
        # print(system, data)
        x_data, y_data = zip(*data)
        pos = [x + (i * bar_width) for x in x_pos]
        bar = ax.bar(pos, y_data, width=bar_width, **PF_MODE_BAR(mode))
        if label:
            bar.set_label(PF_MODE_NAME[mode])

    xticks = BAR_X_TICKS_POS(bar_width, num_bars, num_xticks)
    ax.set_xticks(xticks)
    ax.set_xticklabels(x_data)

    ax.set_title(title, fontsize=18)
    
    ax.set_ylim(0, 50)
    ax.set_yticks(range(0, 51, 25))


if __name__ == '__main__':
    result_path, plot_dir = INIT(sys.argv)

    read_runs = get_runs_from_results(result_path, "prefetch_reads", {})
    read_data = get_data_from_runs(read_runs, "access_size", "bandwidth")
    
    a_read_data = {system: data for system, data in read_data.items() if 'a-256' in system}
    b_read_data = {system: data for system, data in read_data.items() if 'b-256' in system}

    a16_data = {system: data[0:4] for system, data in a_read_data.items()}
    a32_data = {system: data[4:] for system, data in a_read_data.items()}

    b16_data = {system: data[0:4] for system, data in b_read_data.items()}
    b32_data = {system: data[4:] for system, data in b_read_data.items()}

    fig, axes = plt.subplots(2, 2, figsize=(DOUBLE_FIG_WIDTH, 4))
    axes16, axes32  = axes
    (a16_ax, a32_ax) = axes16
    (b16_ax, b32_ax) = axes32

    plot_read(a16_data, a16_ax, "a) A-256 -- 16 Threads", label=True)
    plot_read(a32_data, a32_ax, "b) A-256 -- 32 Threads")
    plot_read(b16_data, b16_ax, "c) B-256 -- 16 Threads")
    plot_read(b32_data, b32_ax, "d) B-256 -- 32 Threads")

    fig.text(0, 0.5, "Throughput (GB/s)", ha='center', va='center', rotation=90)
    fig.text(0.5, -0., "Access Size in Byte", ha='center')

    for ax in (a16_ax, b16_ax, a32_ax, b32_ax):
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "prefetch_reads")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
