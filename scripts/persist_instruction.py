import sys
import matplotlib.pyplot as plt

from common import *


def plot_seq(system_data, ax):
    ops = ['NoCache', 'None']
    num_bars = len(system_data)
    bar_width = 0.8 / num_bars
    x_pos = range(len(ops))

    for i, (system, data) in enumerate(sorted(system_data.items())):
        # print(system, data)
        x_data, y_data = zip(*data)
        bar = BAR(system)
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, **bar, label=SYSTEM_NAME[system])
        if 'dram' in system:
            color = SYSTEM_COLOR[system]
            ax.text(pos[0] + 0.23, 17, int(y_data[0]), ha='center', color=color)
            ax.text(pos[1] - 0.23, 17, int(y_data[1]), ha='center', color=color)

    x_ticks = ops
    assert len(x_ticks) == len(x_data)
    assert all([x.lower() == custom_x.lower() for (x, custom_x) in zip(x_ticks, x_data)])
    x_ticks = [f"$\it{x}$" for x in x_ticks]

    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)

    ax.set_ylabel("Throughput\n(GB/s)")

    ax.set_ylim(0, 22)
    ax.set_yticks(range(0, 22, 5))

    ax.set_title("a) Seq. Writes")


def plot_random(system_data, ax):
    ops = ['nocache', 'cache', 'cacheinv', 'none']
    num_bars = len(system_data)
    bar_width = 0.8 / num_bars
    x_pos = range(len(ops))

    for i, (system, data) in enumerate(sorted(system_data.items())):
        data = sorted(data, key=lambda op: ops.index(op[0]))
        print(system, data)
        x_data, y_data = zip(*data)
        color = SYSTEM_COLOR[system]
        bar = BAR(system)
        pos = [x + (i * bar_width) for x in x_pos]
        ax.bar(pos, y_data, width=bar_width, **bar)
        if 'dram' in system:
            color = SYSTEM_COLOR[system]
            ax.text(pos[0] + 0.23, 5.3, int(y_data[0]), ha='center', color=color)
            ax.text(pos[1] + 0.2,  5.3, int(y_data[1]), ha='center', color=color)
            ax.text(pos[2] + 0.2,  5.3, int(y_data[2]), ha='center', color=color)
            ax.text(pos[3] + 0.0,  6.7, int(y_data[3]), ha='center', color=color)

    op_names = ['NoCache', 'Cache', 'CacheInv', 'None']
    x_ticks = op_names
    assert len(x_ticks) == len(x_data)
    assert all([x.lower() == custom_x.lower() for (x, custom_x) in zip(x_ticks, x_data)])
    x_ticks = [f"$\it{x}$" for x in x_ticks]

    x_ticks_pos = BAR_X_TICKS_POS(bar_width, num_bars, len(x_ticks))
    ax.set_xticks(x_ticks_pos)
    ax.set_xticklabels(x_ticks)

    ax.set_ylim(0, 6.5)
    ax.set_yticks(range(0, 7, 2))

    ax.set_title("b) Random Writes")



if __name__ == '__main__':
    skip_dram = False
    show_prices = True
    result_path, plot_dir = INIT(sys.argv)

    sequential_config = {"access_size": 512, "number_threads": 32}
    sequential_runs = get_runs_from_results(result_path, "sequential_writes", sequential_config, skip_dram=skip_dram)
    sequential_data = get_data_from_runs(sequential_runs, "persist_instruction", "bandwidth")

    random_config = {"number_threads": 32, "access_size": 64}
    random_runs = get_runs_from_results(result_path, "random_writes", random_config, skip_dram=skip_dram)
    random_data = get_data_from_runs(random_runs, "persist_instruction", "bandwidth")

    fig, axes = plt.subplots(1, 2, figsize=(DOUBLE_FIG_WIDTH, 2.5), gridspec_kw={'width_ratios': [1, 2]})
    (seq_ax, random_ax) = axes

    plot_seq(sequential_data, seq_ax)
    plot_random(random_data, random_ax)

    if show_prices:
        print("Random write prices", calc_prices(random_data))

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)

    plot_path = os.path.join(plot_dir, "persist_instruction")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()