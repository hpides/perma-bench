import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *


def plot_data(system_data, ax, label=False):
    # Only use thread count == 32
    system_data = {sys: [(thread, y) for thread, y in data if thread == 32] for sys, data in system_data.items()}

    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data = system_data[system]
        for j, (num_threads, y_data) in enumerate(data):
            y_data = y_data / MILLION
            pos = j + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])


def plot_join_build(system_data, ax):
    plot_data(system_data, ax, label=True)

    ax.set_xticks([])
    ax.set_xticklabels([])

    ax.set_ylim(0, 100)
    ax.set_yticks(range(0, 101, 20))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) Join\nBuild")
    # ax.set_xlabel("32 Threads")

def plot_join_probe(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    ax.set_ylim(0, 150)
    ax.set_yticks(range(0, 151, 30))

    ax.set_title("b) Join\nProbe")
    # ax.set_xlabel("32 Threads")

def plot_index_update(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    ax.set_ylim(0, 40)
    ax.set_yticks(range(0, 41, 10))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title("c) Index\nUpdate")
    # ax.set_xlabel("32 Threads")

def plot_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    ax.set_ylim(0, 60)
    ax.set_yticks(range(0, 61, 15))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title("d) Index\nLookup")
    # ax.set_xlabel("32 Threads")


if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    join_build_config = {"custom_operations": "r256,w64_cache,w64_cache"}
    join_build_runs = get_runs_from_results(result_path, "join_build", join_build_config, skip_dram=skip_dram)
    join_build_data = get_data_from_runs(join_build_runs, "number_threads", "ops_per_second")

    join_probe_config = {"custom_operations": "r512"}
    join_probe_runs = get_runs_from_results(result_path, "join_probe", join_probe_config, skip_dram=skip_dram)
    join_probe_data = get_data_from_runs(join_probe_runs, "number_threads", "ops_per_second")

    index_update_config = {"custom_operations": "r512,r512,r512,w64_cache,w64_cache,w64_cache,w64_cache"}
    index_update_runs = get_runs_from_results(result_path, "primary_index_update", index_update_config, skip_dram=skip_dram)
    index_update_data = get_data_from_runs(index_update_runs, "number_threads", "ops_per_second")

    index_lookup_config = {"custom_operations": "r512,r512,r512"}
    index_lookup_runs = get_runs_from_results(result_path, "primary_index_lookup", index_lookup_config, skip_dram=skip_dram)
    index_lookup_data = get_data_from_runs(index_lookup_runs, "number_threads", "ops_per_second")

    fig, axes = plt.subplots(1, 4, figsize=DOUBLE_FIG_SIZE)
    join_build_ax, join_probe_ax, index_update_ax, index_lookup_ax  = axes
    plot_join_build(join_build_data, join_build_ax)
    plot_join_probe(join_probe_data, join_probe_ax)
    plot_index_update(index_update_data, index_update_ax)
    plot_index_lookup(index_lookup_data, index_lookup_ax)

    HATCH_WIDTH()

    FIG_LEGEND(fig)
    # fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=3,
    #            frameon=False, columnspacing=0.4, handletextpad=0.1,
    #            borderpad=0.1, labelspacing=0.1, handlelength=1.8)
    # fig.tight_layout()


    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "custom_operations")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()