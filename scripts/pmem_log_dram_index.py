import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

import numpy as np

from common import *

def plot_pmem(system_data, ax, label=False):
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
    ax.set_xticklabels(["8/8", "8/16", "16/8", "16/16"])
    ax.set_xlabel("\# PMem/DRAM Threads")

if __name__ == '__main__':
    filter_config = {"pmem_log": { "access_size": 512 }}

    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "pmem_log_dram_index", filter_config, skip_dram=skip_dram, is_parallel=True)

    bw_data = get_data_from_parallel_runs(runs, 
        "pmem_log", "number_threads", "bandwidth",
        "dram_index", "number_threads", "bandwidth")

    pmem_data = {}
    dram_data = {}
    for sys in bw_data:
        data = [(pmem['pmem_log'], dram['dram_index']) for pmem, dram in bw_data[sys]]
        data = [(p, d) for p, d in data if p[0] >= 8 and d[0] >= 8]
        data = list(zip(*data))
        pmem_data[sys] = data[0]
        dram_data[sys] = data[1]

    fig, (pmem_ax, dram_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_pmem(pmem_data, pmem_ax, True)
    plot_pmem(dram_data, dram_ax)

    pmem_ax.set_title("a) PMem Log")
    dram_ax.set_title("b) DRAM Index")

    pmem_ax.set_ylabel("Throughput (GB/s)")

    pmem_ax.set_ylim(0, 17)
    pmem_ax.set_yticks(range(0, 17, 3))

    dram_ax.set_ylim(0, 23)
    dram_ax.set_yticks(range(0, 23, 5))


    HATCH_WIDTH()
    for ax in (pmem_ax, dram_ax):   
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "pmem_log_dram_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
