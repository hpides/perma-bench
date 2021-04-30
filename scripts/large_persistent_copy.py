import sys
import matplotlib.pyplot as plt

from common import *

def plot(system_data, plot_dir):
    fig, ax = plt.subplots(1, 1, figsize=SINGLE_FIG_SIZE)

    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars
    x_pos = range(num_bars)

    for i, (system, data) in enumerate(sorted(system_data.items())):
        _, y_data = zip(*data)
        pos = [x + (i * bar_width) for x in range(len(y_data))]
        ax.bar(pos, y_data, label=SYSTEM_NAME[system], width=bar_width, **BAR(system))

    x_ticks = ['nocache', 'none']
    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)
    ax.set_xlabel("Persist Instruction")

    ax.set_ylabel("Bandwidth (GB/s)")
    ax.set_ylim(0, 22)

    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.13), ncol=5,
               frameon=False, columnspacing=1, handletextpad=0.3)

    HATCH_WIDTH()
    Y_GRID(ax)
    HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "large_persistent_copy")
    SAVE_PLOT(plot_path)


if __name__ == '__main__':
    filter_config = {
        "number_threads": 4,
        "access_size": 1073741824
    }

    result_path, plot_dir = INIT(sys.argv)
    runs = get_runs_from_results(result_path, "large_persistent_copy", filter_config)
    data = get_data_from_runs(runs, "persist_instruction", "bandwidth", "write")

    plot(data, plot_dir)
    PRINT_PLOT_PATHS()
