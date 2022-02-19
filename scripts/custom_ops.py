import sys
import os
from unittest import skip
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

LETTERS = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']
LETTER_ITER = iter(LETTERS)

def plot_data(system_data, ax, label=False):
    # Only use thread count == 32
    system_data = {sys: [(thread, y) for thread, y in data if thread == 32] for sys, data in system_data.items()}

    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data = system_data[system]
        # print(system, data)

        for j, (_, y_data) in enumerate(data):
            y_data = y_data / MILLION
            pos = j + (i * bar_width)
            bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])


def plot_hash_index_update(system_data, ax):
    plot_data(system_data, ax, label=True)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 37
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 8))

    ax.set_ylabel("Million Ops/s")
    ax.set_title(f"{next(LETTER_ITER)}) Hash Index\nUpdate")
    # ax.set_xlabel("32 Threads")

def plot_aggregation(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 37
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 8))

    ax.set_title(f"{next(LETTER_ITER)}) Volatile Hash\nIndex Update")
    # ax.set_xlabel("32 Threads")

def plot_index_update(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 19
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 4))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title(f"{next(LETTER_ITER)}) Tree Index\nUpdate (PMem)")
    # ax.set_xlabel("32 Threads")

def plot_hybrid_index_update(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 19
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 4))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title(f"{next(LETTER_ITER)}) Tree Index\nUpdate (Hybrid)")
    # ax.set_xlabel("32 Threads")

def plot_hash_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 84
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 15))

    ax.set_title(f"{next(LETTER_ITER)}) Hash Index\nLookup")
    # ax.set_xlabel("32 Threads")

def plot_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 39
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 8))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title(f"{next(LETTER_ITER)}) Tree Index\nLookup (PMem)")
    # ax.set_xlabel("32 Threads")

def plot_hybrid_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 39
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 8))

    # ax.set_ylabel("Million Ops/s")
    ax.set_title(f"{next(LETTER_ITER)}) Tree Index\nLookup (Hybrid)")
    # ax.set_xlabel("32 Threads")


if __name__ == '__main__':
    skip_dram = False
    result_path, plot_dir = INIT(sys.argv)

    hash_index_update_config = {"custom_operations": "rp_512,wp_64_cache_128,wp_64_cache_-128"}
    hash_index_update_runs = get_runs_from_results(result_path, "hash_index_update", hash_index_update_config, skip_dram=skip_dram)
    hash_index_update_data = get_data_from_runs(hash_index_update_runs, "number_threads", "ops_per_second")

    hash_index_lookup_config = {"custom_operations": "rp_512"}
    hash_index_lookup_runs = get_runs_from_results(result_path, "hash_index_lookup", hash_index_lookup_config, skip_dram=skip_dram)
    hash_index_lookup_data = get_data_from_runs(hash_index_lookup_runs, "number_threads", "ops_per_second")

    aggregation_config = {"custom_operations": "rp_512,wp_64_none_128,wp_64_none_-128"}
    aggregation_runs = get_runs_from_results(result_path, "hash_aggregation", aggregation_config, skip_dram=skip_dram)
    aggregation_data = get_data_from_runs(aggregation_runs, "number_threads", "ops_per_second")

    index_update_config = {"custom_operations": "rp_512,rp_512,rp_512,wp_64_cache_320,wp_64_cache_-64,wp_64_cache_-64,wp_64_cache_-64"}
    index_update_runs = get_runs_from_results(result_path, "tree_index_update", index_update_config, skip_dram=skip_dram)
    index_update_data = get_data_from_runs(index_update_runs, "number_threads", "ops_per_second")

    index_lookup_config = {"custom_operations": "rp_512,rp_512,rp_512"}
    index_lookup_runs = get_runs_from_results(result_path, "tree_index_lookup", index_lookup_config, skip_dram=skip_dram)
    index_lookup_data = get_data_from_runs(index_lookup_runs, "number_threads", "ops_per_second")

    hybrid_index_update_config = {"custom_operations": "rd_2048,rd_2048,rp_1024,wp_64_cache_512,wp_64_cache_-512,wp_64_cache"}
    hybrid_index_update_runs = get_runs_from_results(result_path, "hybrid_tree_index_update", hybrid_index_update_config, skip_dram=skip_dram)
    hybrid_index_update_data = get_data_from_runs(hybrid_index_update_runs, "number_threads", "ops_per_second")

    hybrid_index_lookup_config = {"custom_operations": "rd_2048,rd_2048,rp_1024"}
    hybrid_index_lookup_runs = get_runs_from_results(result_path, "hybrid_tree_index_lookup", hybrid_index_lookup_config, skip_dram=skip_dram)
    hybrid_index_lookup_data = get_data_from_runs(hybrid_index_lookup_runs, "number_threads", "ops_per_second")

    fig, axes = plt.subplots(1, 7, figsize=(2 * DOUBLE_FIG_WIDTH, DOUBLE_FIG_HEIGHT))
    # hash_index_update_ax, hash_index_lookup_ax, aggregation_ax, index_update_ax, index_lookup_ax, hybrid_index_update_ax, hybrid_index_lookup_ax  = axes
    axes_iter = iter(axes)

    plot_hash_index_update(hash_index_update_data, next(axes_iter))
    plot_aggregation(aggregation_data, next(axes_iter))
    plot_index_update(index_update_data, next(axes_iter))
    plot_hybrid_index_update(hybrid_index_update_data, next(axes_iter))
    plot_hash_index_lookup(hash_index_lookup_data, next(axes_iter))
    plot_index_lookup(index_lookup_data, next(axes_iter))
    plot_hybrid_index_lookup(hybrid_index_lookup_data, next(axes_iter))

    all_data = (hash_index_update_data, aggregation_data, index_update_data, hybrid_index_update_data,
                hash_index_lookup_data, index_lookup_data, hybrid_index_lookup_data) 

    dist = 0.1393
    for i, data in enumerate(all_data, 1):
        dram_sys = 'z-barlow-dram'
        print('check dram values')
        dram_y = int(data[dram_sys][-2][1] / MILLION)
        len_y = len(f"{dram_y}")
        offset = -0.002 if len_y == 3 else 0
        fig.text((i * dist) + offset, 0.64, dram_y, ha='left', color=SYSTEM_COLOR[dram_sys],
                 bbox=dict(boxstyle='square,pad=0.05', fc='white', ec=SYSTEM_COLOR[dram_sys]))


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