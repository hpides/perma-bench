import sys
import os
from unittest import skip
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

LETTERS = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']
LETTER_ITER = iter(LETTERS)

THREAD_COUNT = 32
THREAD_POS = -1

def plot_data(system_data, ax, label=False):
    # Only use thread count == THREAD_COUNT
    system_data = {sys: [(thread, y) for thread, y in data if thread == THREAD_COUNT] for sys, data in system_data.items()}

    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    dram_data = system_data[bars[-1]]

    for i, system in enumerate(bars):
        data = system_data[system]

        for j, (_, y_data) in enumerate(data):
            y_diff = dram_data[j][1] / y_data 
            pos = j + (i * bar_width)
            print(system, y_diff)
            bar = ax.bar(pos, y_diff, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])


def plot_hash_index_update(system_data, ax):
    plot_data(system_data, ax, label=True)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 7
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 2))

    ax.set_ylabel("Factor lower\nthan DRAM")
    ax.set_xlabel(f"{next(LETTER_ITER)}) Up.")
    # ax.set_xlabel("32 Threads")

def plot_aggregation(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 11
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 3))

    ax.set_xlabel(f"{next(LETTER_ITER)}) Volatile Hash\nUpdate")
    # ax.set_xlabel("32 Threads")

def plot_index_update(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 5
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 2))

    # ax.set_ylabel("Million Ops/s")
    ax.set_xlabel(f"{next(LETTER_ITER)}) Up.")
    # ax.set_xlabel("32 Threads")

def plot_hybrid_index_update(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 2.5
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, 3, 1))

    # ax.set_ylabel("Million Ops/s")
    ax.set_xlabel(f"{next(LETTER_ITER)}) Up.")
    # ax.set_xlabel("32 Threads")

def plot_hash_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 7
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, max_y + 1, 2))

    ax.set_xlabel(f"{next(LETTER_ITER)}) Lk.")
    # ax.set_xlabel("32 Threads")

def plot_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 5 
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, 5, 2))

    # ax.set_ylabel("Million Ops/s")
    ax.set_xlabel(f"{next(LETTER_ITER)}) Lk.")
    # ax.set_xlabel("32 Threads")

def plot_hybrid_index_lookup(system_data, ax):
    plot_data(system_data, ax)

    ax.set_xticks([])
    ax.set_xticklabels([])

    max_y = 2.5
    ax.set_ylim(0, max_y)
    ax.set_yticks(range(0, 3, 1))

    # ax.set_ylabel("Million Ops/s")
    ax.set_xlabel(f"{next(LETTER_ITER)}) Lk.")
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

    print(hybrid_index_update_data)

    # fig, axes = plt.subplots(1, 7, figsize=(2 * DOUBLE_FIG_WIDTH, 3))
    fig, axes = plt.subplots(1, 6, figsize=(1 * DOUBLE_FIG_WIDTH, 2.5))
    axes_iter = iter(axes)

    plot_hash_index_update(hash_index_update_data, next(axes_iter))
    # plot_aggregation(aggregation_data, next(axes_iter))
    plot_hash_index_lookup(hash_index_lookup_data, next(axes_iter))
    plot_index_update(index_update_data, next(axes_iter))
    plot_index_lookup(index_lookup_data, next(axes_iter))
    plot_hybrid_index_update(hybrid_index_update_data, next(axes_iter))
    plot_hybrid_index_lookup(hybrid_index_lookup_data, next(axes_iter))

    HATCH_WIDTH()

    FIG_LEGEND(fig)

    fig.text(0.23, 0, "Hash Index", va='center', ha='center')
    fig.text(0.53, 0, "Tree Index (PMem)", va='center', ha='center')
    fig.text(0.85, 0, "Tree Index (Hybrid)", va='center', ha='center')

    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "ops_to_dram")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()