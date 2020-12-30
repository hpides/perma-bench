import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

from mat_json_utils import MatrixJsonUtils


class MatrixJsonPlotter:
    def __init__(self, img_dir, path):
        self.img_dir = img_dir
        self.utils = MatrixJsonUtils(path)
        self.results = defaultdict(list)

    def __del__(self):
        del self.utils

    def set_values_of_current_bm(self, bm_idx):
        for field, values in self.utils.results.items():
            self.results[field] = values[bm_idx]

    """
        helper functions:
    """

    def get_indices_of_legend_categories(self, legend_values):
        indices_per_legend_category = defaultdict(list)
        for idx in range(len(legend_values)):
            if indices_per_legend_category[legend_values[idx]]:
                indices_per_legend_category[legend_values[idx]].append(idx)
            else:
                indices_per_legend_category[legend_values[idx]] = [idx]
        return indices_per_legend_category

    def get_max_number_of_indices(self, indices_per_legend_category):
        return max(len(v) for v in indices_per_legend_category.values())

    def save_png(self, y_label, perm):
        bm_name = self.results["bm_name"]
        y_name = y_label.lower().replace(" ", "_").split("_(")[0]
        plt.savefig(f"{self.img_dir}{bm_name}-{perm[0]}-{perm[1]}-{y_name}.png")

    """
        internal plotting functions:
    """

    def plot_categorical_x_and_legend(self, perm, y_value, y_label):
        x_values = self.results[perm[0]]
        legend_values = self.results[perm[1]]
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)
        max_num_indices = self.get_max_number_of_indices(indices_per_legend_category)

        # collect y-values for each legend category
        y_values_per_legend_category = defaultdict(list)
        x_categories = list()
        for i, cat in enumerate(sorted(indices_per_legend_category.keys())):
            # create lexicographically ordered list of x-categories for correct y-value retrieval and x-ticks
            if i == 0:
                x_categories = sorted(list(set(x_values)))

            # init size of y-values dict
            y_values_per_legend_category[cat] = [0] * max_num_indices

            # fill y-values dict
            for idx in indices_per_legend_category[cat]:
                # ensure that dict is filled according to the order of categories on x-axis
                pos = x_categories.index(x_values[idx])
                y_values_per_legend_category[cat][pos] = self.results[y_value][idx]

        # actual plot
        x_pos = np.arange(len(x_categories))
        bar_width = 0.25
        fig, ax = plt.subplots()
        for j, cat in enumerate(y_values_per_legend_category.keys()):
            rects = ax.bar(x_pos + j*bar_width, y_values_per_legend_category[cat], bar_width, label=cat)

            # draw cross if y-value is null
            for rect in rects:
                if rect.get_height() == 0:
                    ax.annotate("X",
                                xy=(rect.get_x() + bar_width/2, 0),
                                xytext=(rect.get_x() + bar_width/2, 0),
                                ha="center",
                                va="baseline",
                                fontsize=18,
                                fontweight='bold',
                                color=plt.gca().containers[j].patches[0].get_facecolor())

        # set text of axes, x-ticks and legend title
        ax.set_xlabel(self.utils.get_arg_label(perm[0]))
        ax.set_ylabel(y_label)
        bars_per_category = len(set(legend_values))
        x_ticks_pos = [x + (((bars_per_category / 2) - 0.5) * bar_width) for x in x_pos]
        ax.set_xticks(x_ticks_pos)
        ax.set_xticklabels(x_categories)
        ax.legend(title=(self.utils.get_arg_label(perm[1])+":"), loc="upper left", bbox_to_anchor=(1, 1))
        fig.tight_layout()

        # save png and close current figure
        self.save_png(y_label, perm)
        plt.close()

    def plot_continuous_x_and_legend(self, perm, y_value, y_label):
        x_values = self.results[perm[0]]
        legend_values = self.results[perm[1]]
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)
        max_num_indices = self.get_max_number_of_indices(indices_per_legend_category)

        # collect values for each legend category
        x_values_per_legend_category = defaultdict(list)
        y_values_per_legend_category = defaultdict(list)
        for i, cat in enumerate(sorted(indices_per_legend_category.keys())):
            for idx in indices_per_legend_category[cat]:
                x_values_per_legend_category[cat].append(x_values[idx])
                y_values_per_legend_category[cat].append(self.results[y_value][idx])

        # actual plot
        for cat in x_values_per_legend_category.keys():
            plt.plot(x_values_per_legend_category[cat], y_values_per_legend_category[cat], "-o", label=cat)

        # set text of axes and legend title
        plt.xlabel(self.utils.get_arg_label(perm[0]))
        plt.ylabel(y_label)
        plt.legend(title=(self.utils.get_arg_label(perm[1])+":"), loc="upper left", bbox_to_anchor=(1, 1))

        # set layout
        plt.xlim(left=0)
        plt.ylim(bottom=0)
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_label, perm)
        plt.close()

    """
        externally called plotting functions:
    """

    def plot_categorical_x_with_two_args(self, perm):
        self.plot_categorical_x_and_legend(perm, "avg", "Average Duration (ns)")
        self.plot_categorical_x_and_legend(perm, "bandwidth_values", "Bandwidth (GB/s)")

    def plot_continuous_x_with_two_args(self, perm):
        self.plot_continuous_x_and_legend(perm, "avg", "Average Duration (ns)")
        self.plot_continuous_x_and_legend(perm, "bandwidth_values", "Bandwidth (GB/s)")

    def plot_categorical_x_with_one_arg(self, arg):
        pass

    def plot_continuous_x_with_one_arg(self, arg):
        pass