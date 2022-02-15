import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

DIMM_COLOR = {
    'dimm1': '#fecc5c', # '#a1dab4',
    'dimm2': '#fd8d3c', # '#378d54',
    'dimm4': '#f03b20', # '#41b6c4',
    'dimm6': '#bd0026', # '#2c7fb8',
}

DIMM_HATCH = {
    'dimm1': '\\\\',
    'dimm2': '//',
    'dimm4': '/',
    'dimm6': '\\',
}

DIMM_NAME = {
    'dimm1': "1 DIMM",
    'dimm2': "2 DIMMs",
    'dimm4': "4 DIMMs",
    'dimm6': "6 DIMMs",
}

def DIMM_BAR(dimm):
    return {
        "color": 'white',
        "edgecolor": DIMM_COLOR[dimm],
        "hatch": DIMM_HATCH[dimm],
        "lw": 3
    }

def plot_dimms(scan_data, logging_data, index_data, ax):
    bars = sorted(scan_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    num_xticks = 3
    x_pos = range(num_xticks)

    def plot_bm(system_data, offset, label=False):
        base_y = 0

        for i, system in enumerate(bars):
            data = system_data[system]
            y_data = data[-1][1]
            if i == 0:
                base_y = y_data

            pos = x_pos[offset] + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **DIMM_BAR(system))
            if label:
                label = DIMM_NAME[system]
                bar.set_label(label)

            if y_data == base_y: diff_str = "1x"
            else: diff_str = f"{(y_data / base_y):0.1f}x"
            ax.text(pos - 0.01, y_data + 1, diff_str, ha='center', color=DIMM_COLOR[system])

    plot_bm(scan_data, 0, label=True)
    plot_bm(logging_data, 1)
    plot_bm(index_data, 2)

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["Table Scan", "Logging", "Index Lookup"])

    ax.set_ylim(0, 45)
    ax.set_yticks(range(0, 45, 10))

    ax.set_ylabel("Throughput (GB/s)")


if __name__ == '__main__':
    result_path, plot_dir = INIT(sys.argv)

    scan_config = {"access_size": 16384}
    scan_runs = get_runs_from_results(result_path, "table_scan_partitioned", scan_config)
    scan_data = get_data_from_runs(scan_runs, "number_threads", "bandwidth", "read")

    logging_config = {"access_size": 512, "persist_instruction": "nocache"}
    logging_runs = get_runs_from_results(result_path, "logging_partition", logging_config)
    logging_data = get_data_from_runs(logging_runs, "number_partitions", "bandwidth", "write")

    index_config = {"access_size": 256, "random_distribution": "uniform"}
    index_runs = get_runs_from_results(result_path, "index_lookup", index_config)
    index_data = get_data_from_runs(index_runs, "number_partitions", "bandwidth", "read")

    fig, ax = plt.subplots(1, 1, figsize=DOUBLE_FIG_SIZE)
    plot_dimms(scan_data, logging_data, index_data, ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "dimm_performance")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()