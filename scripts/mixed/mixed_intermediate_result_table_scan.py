import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

import numpy as np

from common import *


def plot_bw_heat(system_data, sub_bm, ax, sub_title, vmax, cmap='Blues', set_clabel=False):

    sub_bm_one_unique_values = []
    sub_bm_two_unique_values = []
    data = np.array([])

    for system, par_data in sorted(system_data.items()):
        for sub_bm_one, sub_bm_two in par_data:
            if sub_bm in sub_bm_one:
                x_data_one, y_data = sub_bm_one[sub_bm]
                x_data_two, _ = zip(*sub_bm_two.values())
                x_data_two = x_data_two[0]
            else:
                x_data_one, _ = zip(*sub_bm_one.values())
                x_data_two, y_data = sub_bm_two[sub_bm]
                x_data_one = x_data_one[0]

            data = np.append(data, np.array([y_data]), axis=0)
            if x_data_one not in sub_bm_one_unique_values:
                sub_bm_one_unique_values += [x_data_one]
            if x_data_two not in sub_bm_two_unique_values:
                sub_bm_two_unique_values += [x_data_two]

    data = data.reshape((len(sub_bm_one_unique_values), len(sub_bm_two_unique_values)))

    ax.set_xticks(np.arange(len(sub_bm_two_unique_values)))
    ax.set_yticks(np.arange(len(sub_bm_one_unique_values)))

    ax.set_xticklabels(sub_bm_two_unique_values)
    ax.set_yticklabels(sub_bm_one_unique_values)

    ax.tick_params(top=False, bottom=True, labeltop=False, labelbottom=True)
    ax.set_title(sub_title)
    plt.setp(ax.get_xticklabels(), ha="center")

    for spine in ax.spines.values():
        spine.set_visible(False)

    hm = ax.imshow(data, cmap=cmap, interpolation="nearest", vmin=0, vmax=vmax)
    cbar = ax.figure.colorbar(hm, ax=ax)
    if set_clabel:
        cbar.ax.set_ylabel("Throughput GB/s", rotation=-90, va="bottom")


if __name__ == '__main__':
    filter_config = {
        # "intermediate_result": { "access_size": 64 },
        # "table_scan_partitioned": { "access_size": 4096 }
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "intermediate_result_table_scan_mixed", filter_config, False, True)

    bw_data = get_data_from_parallel_runs(runs, "table_scan", "number_threads", "bandwidth", "read",
                                          "intermediate_result", "number_threads", "bandwidth", "write")

    fig, (bw_ax_one, bw_ax_two) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_bw_heat(bw_data, "table_scan", bw_ax_one, "a) Table Scan", 40, 'YlOrRd')
    plot_bw_heat(bw_data, "intermediate_result", bw_ax_two, "b) Intermediate Result", 10, 'BuGn', True)

    bw_ax_one.set_xlabel("\# Result Threads")
    bw_ax_two.set_xlabel("\# Result Threads")
    bw_ax_one.set_ylabel("\# Scan Threads")
    # bw_ax_two.set_ylabel("\# Scan Threads")

    plot_path = os.path.join(plot_dir, "table_scan_intermediate_result_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()
