import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

SPEED_COLOR = {
    "speed2133":      '#fecc5c',
    "speed2400":      '#fd8d3c',
    "speed2666":      '#f03b20',
    "speed2133-dram": '#bd0026',
    "speed2666-dram": '#bd0026',
}

SPEED_NAME = {
    "speed2133": '2133',
    "speed2400": '2400',
    "speed2666": '2666',
    "speed2133-dram": '2133-DRAM',
    "speed2666-dram": '2666-DRAM',
}

SPEED_HATCH = {
    "speed2133": '\\\\',
    "speed2400": '//',
    "speed2666": '\\',
    "speed2133-dram": '//',
    "speed2666-dram": 'x',
}

def SPEED_BAR(system):
    return {
        "color": 'white',
        "edgecolor": SPEED_COLOR[system],
        "hatch": SPEED_HATCH[system],
        "lw": 3
    }

def plot_dimms(scan_data, logging_data, index_data, ax):
    bars = sorted(scan_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = 3
    x_pos = range(num_xticks)

    def plot_bm(system_data, offset, label=False):
        for i, system in enumerate(bars):
            data = system_data[system]
            y_data = data[-1][1]
            # print(f"{system}: {y_data}")
            pos = x_pos[offset] + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **SPEED_BAR(system))
            if label:
                label = SPEED_NAME[system]
                bar.set_label(label)

    plot_bm(scan_data, 0, label=True)
    plot_bm(logging_data, 1)
    plot_bm(index_data, 2)

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["Table\nScan", "Log-\nging", "Index\nLookup"])

    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))

    ax.set_ylabel("Throughput (GB/s)")


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    scan_config = {"access_size": 16384}
    scan_runs = get_runs_from_results(result_path, "table_scan_partitioned", scan_config, skip_dram=skip_dram)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth", "read")

    logging_config = {"access_size": 512, "persist_instruction": "nocache"}
    logging_runs = get_runs_from_results(result_path, "logging_partition", logging_config, skip_dram=skip_dram)
    logging_data = get_data_from_runs(logging_runs, "number_partitions", "bandwidth", "write")

    index_config = {"access_size": 256, "random_distribution": "uniform"}
    index_runs = get_runs_from_results(result_path, "index_lookup", index_config, skip_dram=skip_dram)
    index_data = get_data_from_runs(index_runs, "number_partitions", "bandwidth", "read")

    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)
    plot_dimms(scan_data, logging_data, index_data, ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "speed_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()