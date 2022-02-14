import sys
import os
import json
from pprint import pprint
from collections import defaultdict
import matplotlib
import matplotlib.pyplot as plt


#######################################
# Plotting
#######################################

FS = 20
MILLION = 1_000_000
SINGLE_FIG_WIDTH = 5
SINGLE_FIG_HEIGHT = 3.5
SINGLE_FIG_SIZE = (SINGLE_FIG_WIDTH, SINGLE_FIG_HEIGHT)
DOUBLE_FIG_WIDTH = 10
DOUBLE_FIG_HEIGHT = 3.5
DOUBLE_FIG_SIZE = (DOUBLE_FIG_WIDTH, DOUBLE_FIG_HEIGHT)
PLOT_PATHS = []
IMG_TYPES = ['.png', '.svg']

SYSTEM_COLOR = {
    'intel-128':   '#a1dab4',
    'intel-256':   '#378d54',
    'intel-512':   '#41b6c4',
    'intel-gen2':  '#2c7fb8',
    'zdram':       '#253494',
    'nvdimm-hpe':  '#0c1652',
}

SYSTEM_MARKER = {
    'intel-128':   '^',
    'intel-256':   'o',
    'intel-512':   'd',
    'intel-gen2':  's',
    'zdram':       'X',
    'nvdimm-hpe':  'x',
}

SYSTEM_HATCH = {
    'intel-128':   '\\\\',
    'intel-256':   '//',
    'intel-512':   '\\',
    'intel-gen2':  '/',
    'zdram':       'x',
    'nvdimm-hpe':  '.',
}

SYSTEM_NAME = {
    'intel-128':   'I-128',
    'intel-256':   'I-256',
    'intel-512':   'I-512',
    'intel-gen2':  'I-Gen2',
    'zdram':       'DRAM',
    'nvdimm-hpe':  'HPE',
}


def INIT_PLOT():
    matplotlib.rcParams.update({
        'font.size': FS,
        'svg.fonttype': 'none',
    })


def PRINT_PLOT_PATHS():
    print(f"To view new plots, run:\n\topen {' '.join(PLOT_PATHS)}")

def BAR(system):
    return {
        "color": 'white',
        "edgecolor": SYSTEM_COLOR[system],
        "hatch": SYSTEM_HATCH[system],
        "lw": 3
    }

def LINE(system):
    return {
        "lw": 4,
        "ms": 10,
        "color": SYSTEM_COLOR[system],
        "marker": SYSTEM_MARKER[system],
        "markeredgewidth": 1,
        "markeredgecolor": 'black',
    }

def BAR_X_TICKS_POS(bar_width, num_bars, num_xticks):
    return [i - (bar_width / 2) + ((num_bars * bar_width) / 2) for i in range(num_xticks)]

def RESIZE_TICKS(ax, x=FS, y=FS):
    for tick in ax.xaxis.get_major_ticks():
        tick.label.set_fontsize(x)
    for tick in ax.yaxis.get_major_ticks():
        tick.label.set_fontsize(y)

def HATCH_WIDTH(width=4):
    matplotlib.rcParams['hatch.linewidth'] = width

def Y_GRID(ax):
    ax.grid(axis='y', which='major')
    ax.set_axisbelow(True)

def HIDE_BORDERS(ax, show_left=False):
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['bottom'].set_visible(True)
    ax.spines['left'].set_visible(show_left)

def FIG_LEGEND(fig):
    fig.legend(loc='upper center', bbox_to_anchor=(0.5, 1.1), ncol=6,
               frameon=False, columnspacing=1, handletextpad=0.3
               #, borderpad=0.1, labelspacing=0.1, handlelength=1.8
              )
    fig.tight_layout()


def SAVE_PLOT(plot_path, img_types=None):
    if img_types is None:
        img_types = IMG_TYPES

    for img_type in img_types:
        img_path = f"{plot_path}{img_type}"
        PLOT_PATHS.append(img_path)
        plt.savefig(img_path, bbox_inches='tight', dpi=300)

    plt.figure()


#######################################
# Benchmark Results
#######################################

def get_bms(results, bm_name, skip_dram=True):
    bms = {}
    for system_results in os.listdir(results):
        if skip_dram and "dram" in system_results:
            continue
        if not system_results.endswith(".json"):
            continue

        with open(os.path.join(results, system_results)) as res_f:
            res_bms = json.loads(res_f.read())
            for res_bm in res_bms:
                if res_bm["bm_name"] == bm_name:
                    system_name = system_results.replace(".json", "")
                    bms[system_name] = res_bm
                    break
    return bms

def get_parallel_runs(bms, config):
    runs = defaultdict(list)
    for name, res_runs in bms.items():
        for run in res_runs["benchmarks"]:
            run_confs = run["configs"]

            append = True
            for single_bm_name, single_bm_conf in config.items():
                if not all(run_confs[single_bm_name][conf_name] == conf_val for conf_name, conf_val in single_bm_conf.items()):
                    append = False

            if append:
                runs[name].append(run)

    return runs


def get_runs(bms, config):
    runs = defaultdict(list)
    for name, res_runs in bms.items():
        for run in res_runs["benchmarks"]:
            run_conf = run["config"]
            if all(run_conf[conf_name] == conf_val for conf_name, conf_val in config.items()):
                runs[name].append(run)
    return runs


def get_runs_from_results(results, bm_name, filter_config, skip_dram=True, is_parallel=False):
    bms = get_bms(results, bm_name, skip_dram)
    if is_parallel:
        return get_parallel_runs(bms, filter_config)
    else:
        return get_runs(bms, filter_config)


def get_data_from_parallel_runs(runs, sub_bm_name_one, x_attribute_one, y_type_one,
                                y_attribute_one, sub_bm_name_two, x_attribute_two, y_type_two, y_attribute_two):
    data = defaultdict(list)
    for system_name, system_runs in runs.items():
        d = data[system_name]
        for run in system_runs:
            x_val_one = run['configs'][sub_bm_name_one][x_attribute_one]
            x_val_two = run['configs'][sub_bm_name_two][x_attribute_two]
            y_val_one = run['results'][sub_bm_name_one][y_type_one].get(y_attribute_one, 0)
            y_val_two = run['results'][sub_bm_name_two][y_type_two].get(y_attribute_two, 0)
            d.append(({sub_bm_name_one: (x_val_one, y_val_one)}, {sub_bm_name_two: (x_val_two, y_val_two)}))

    return data


def get_data_from_runs(runs, x_attribute, y_type, y_attribute=None):
    data = defaultdict(list)
    for system_name, system_runs in runs.items():
        d = data[system_name]
        for run in system_runs:
            x_val = run['config'][x_attribute]
            if y_attribute is None:
                y_val = run['results'][y_type]
            else:
                y_val = run['results'][y_type].get(y_attribute, 0)
            d.append((x_val, y_val))
    return data


def INIT(args):
    if len(args) != 3:
        sys.exit("Need /path/to/results /path/to/plots")

    result_path = args[1]
    plot_dir = args[2]

    os.makedirs(plot_dir, exist_ok=True)
    INIT_PLOT()

    return result_path, plot_dir