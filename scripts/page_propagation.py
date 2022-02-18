import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))
import matplotlib.pyplot as plt

from common import *


def plot_page(system_data, ax, label=False):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = len(list(system_data.items())[0][1])

    for i, system in enumerate(bars):
        data = system_data[system]
        # for j, (_, y_data) in enumerate(data):
        y_data = [y for _, y in data]
        pos = [j + (i * bar_width) for j in range(len(data))]
        bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["256/256", "256/4K", "4K/4K"])
    ax.set_xlabel("Page Size In/Out in Bytes")


if __name__ == '__main__':
    filter_config = {}
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "page_propagation", filter_config, skip_dram=skip_dram, is_parallel=True)

    bw_data = get_data_from_parallel_runs(runs, 
        "page_in", "access_size", "bandwidth",
        "page_out", "access_size", "bandwidth")

    page_in_data = {}
    page_out_data = {}
    for sys in bw_data:
        data = [(pmem['page_in'], dram['page_out']) for pmem, dram in bw_data[sys]]
        data = [(in_size, out_size) for in_size, out_size in data if in_size[0] <= out_size[0] and out_size[0] <= 4096]
        data = list(zip(*data))
        page_in_data[sys] = data[0]
        page_out_data[sys] = data[1]

    fig, (page_in_ax, page_out_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_page(page_in_data, page_in_ax, True)
    plot_page(page_out_data, page_out_ax)

    page_in_ax.set_title("a) Page In")
    page_out_ax.set_title("b) Page Out")

    page_in_ax.set_ylabel("Throughput (GB/s)")

    page_in_ax.set_ylim(0, 30)
    page_in_ax.set_yticks(range(0, 31, 5))

    page_out_ax.set_ylim(0, 30)
    page_out_ax.set_yticks(range(0, 31, 5))


    HATCH_WIDTH()
    for ax in (page_in_ax, page_out_ax):   
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "page_propagation")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
