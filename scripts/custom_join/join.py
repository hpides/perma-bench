import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

DASH_BUILD = {
    "intel-128": [(16, 10324116)],
    "intel-256": [(16, 13550466)]
}

DASH_PROBE = {
    "intel-128": [(16, 38587320)],
    "intel-256": [(16, 47271039)]
}


def plot_data(system_data, ax, offset=0, label=False):
    bars = sorted(system_data.keys(), reverse=True)
    num_bars = len(bars)
    bar_width = 0.6 / num_bars

    num_xticks = 2
    x_pos = range(num_xticks)

    first_bar_y = 0
    for i, system in enumerate(bars):
        data = system_data[system]
        if (len(data) == 1):
            xy_data = data[0]
        else:
            xy_data = data[2]

        assert xy_data[0] == 16
        y_data = xy_data[1] / MILLION
        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

        if i == 0:
            first_bar_y = y_data

        if i == 1:
            diff = int((y_data / first_bar_y) * 100)
            ax.text(pos, y_data + 1, f"{diff}\%", ha="center", size=FS - 2)

    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))
    ax.set_xticklabels(["PerMA", "Dash"])


def plot_join_build(system_data, ax):
    plot_data(system_data, ax, offset=0, label=True)
    plot_data(DASH_BUILD, ax, offset=1)

    ax.set_ylim(0, 40)
    ax.set_yticks(range(0, 41, 10))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) Join Build")

def plot_join_probe(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(DASH_PROBE, ax, offset=1)

    ax.set_ylim(0, 50)
    ax.set_yticks(range(0, 51, 10))
    ax.set_title("b) Join Probe")


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    build_config = {"custom_operations": "r256,w64_cache,w64_cache"}
    build_runs = get_runs_from_results(result_path, "join_build", build_config, skip_dram=skip_dram)
    build_data = get_data_from_runs(build_runs, "number_threads", "ops_per_second")

    probe_config = {"custom_operations": "r512"}
    probe_runs = get_runs_from_results(result_path, "join_probe", probe_config, skip_dram=skip_dram)
    probe_data = get_data_from_runs(probe_runs, "number_threads", "ops_per_second")

    fig, (build_ax, probe_ax) = plt.subplots(1, 2, figsize=DOUBLE_FIG_SIZE)
    plot_join_build(build_data, build_ax)
    plot_join_probe(probe_data, probe_ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in (build_ax, probe_ax):
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_join")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()