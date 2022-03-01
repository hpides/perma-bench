import sys
import os
sys.path.append(os.path.dirname(sys.path[0]))

from common import *

FPTREE_DATA = {
    "apache-256": [(16, 8812391), (32, 14676720)],
    "barlow-256": [(16, 8538416), (32, 16864234)],
}

LBTREE_DATA = {
    "apache-256": [(16, 13925829), (32, 23367677)],
    "barlow-256": [(16, 12828513), (32, 25442007)],
}

def plot_data(system_data, ax, offset=0, label=False):
    bars = sorted(system_data.keys())
    num_bars = len(bars)
    bar_width = 0.8 / num_bars

    for i, system in enumerate(bars):
        data = system_data[system] 
        # print(system, data)
        xy_data = data[1]
        y_data = xy_data[1] / MILLION
        pos = offset + (i * bar_width)
        bar = ax.bar(pos, y_data, width=bar_width, **BAR(system))
        if label:
            bar.set_label(SYSTEM_NAME[system])

    num_xticks = 2
    ax.set_xticks(BAR_X_TICKS_POS(bar_width, num_bars, num_xticks))


def plot_fptree(system_data, ax):
    plot_data(system_data, ax, offset=0, label=True)
    plot_data(FPTREE_DATA, ax, offset=1)

    ax.set_ylim(0, 40)
    ax.set_yticks(range(0, 41, 15))

    ax.set_ylabel("Million Ops/s")
    ax.set_title("a) FPTree")
    ax.set_xticklabels(["PerMA", "FPTree"])

def plot_lbtree(system_data, ax):
    plot_data(system_data, ax, offset=0)
    plot_data(LBTREE_DATA, ax, offset=1)

    ax.set_ylim(0, 52)
    ax.set_yticks(range(0, 52, 15))
    ax.set_title("b) LB+Tree")
    ax.set_xticklabels(["PerMA", "LB+Tree"])


if __name__ == '__main__':
    skip_dram = True
    result_path, plot_dir = INIT(sys.argv)

    fptree_config = {"custom_operations": "rd_2048,rd_2048,rp_1024"}
    fptree_runs = get_runs_from_results(result_path, "hybrid_tree_index_lookup", fptree_config, skip_dram=skip_dram)
    fptree_data = get_data_from_runs(fptree_runs, "number_threads", "ops_per_second")

    # lbtree_config = {"custom_operations": "rd_512,rd_512,rd_512,rp_256"}
    lbtree_config = {"number_threads": 32}
    lbtree_runs = get_runs_from_results(result_path, "lbtree_lookup", lbtree_config, skip_dram=skip_dram)
    # lbtree_data = get_data_from_runs(lbtree_runs, "number_threads", "ops_per_second")
    lbtree_data = get_data_from_runs(lbtree_runs, "custom_operations", "ops_per_second")

    pprint(lbtree_data)

    lbtree_data['apache-256'] = lbtree_data['apache-256-lbtree']
    lbtree_data['barlow-256'] = lbtree_data['barlow-256-lbtree']
    del lbtree_data['apache-256-lbtree']
    del lbtree_data['barlow-256-lbtree']

    fig, axes = plt.subplots(1, 2, figsize=(DOUBLE_FIG_WIDTH, 2.5))
    fptree_ax, lbtree_ax = axes

    fptree_data = {s: data[-2:] for s, data in fptree_data.items()}
    lbtree_data = {s: data[-2:] for s, data in lbtree_data.items()}

    plot_fptree(fptree_data, fptree_ax)
    plot_lbtree(lbtree_data, lbtree_ax)

    HATCH_WIDTH()
    FIG_LEGEND(fig)
    for ax in axes:
        Y_GRID(ax)
        HIDE_BORDERS(ax)

    plot_path = os.path.join(plot_dir, "hybrid_tree_index")
    SAVE_PLOT(plot_path)
    PRINT_PLOT_PATHS()