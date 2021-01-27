import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

from mat_json_reader import MatrixJsonReader


class MatrixJsonPlotter:
    def __init__(self, img_dir, path):
        self.img_dir = img_dir
        self.reader = MatrixJsonReader(path)

    """
        helper functions:
    """

    def get_indices_of_legend_categories(self, legend_values):
        indices_per_legend_category = defaultdict(list)
        for idx in range(len(legend_values)):
            if not indices_per_legend_category[legend_values[idx]]:
                indices_per_legend_category[legend_values[idx]] = list()
            indices_per_legend_category[legend_values[idx]].append(idx)

        return indices_per_legend_category

    def save_png(self, y_value, args, bm_idx):
        bm_name = self.reader.get_result("bm_name", bm_idx)

        if y_value == "avg":
            y_name = "average_duration"
        elif y_value == "bandwidth_values":
            y_name = "bandwidth"
        else:
            y_name = y_value

        if isinstance(args, tuple):
            plt.savefig(f"{self.img_dir}{bm_name}-{args[0]}-{args[1]}-{y_name}.png")
        else:
            plt.savefig(f"{self.img_dir}{bm_name}-{args}-{y_name}.png")

    """
        plotting functions:
    """

    def plot_duration_boxes(self, arg, bm_idx):
        x_categories = self.reader.get_result(arg, bm_idx)
        num_boxes = len(x_categories)

        # get and set statistical values
        medians = self.reader.get_result("median", bm_idx)
        lower_quartiles = self.reader.get_result("lower_quartile", bm_idx)
        upper_quartiles = self.reader.get_result("upper_quartile", bm_idx)
        mins = self.reader.get_result("min", bm_idx)
        maxes = self.reader.get_result("max", bm_idx)
        stats = [{'whislo': mins[i], 'q1': lower_quartiles[i], 'med': medians[i], 'q3': upper_quartiles[i],
                          'whishi': maxes[i]} for i in range(num_boxes)]

        # actual boxplot
        fig, ax = plt.subplots()
        ax.bxp(stats, showfliers=False)

        # set properties of axes
        ax.set_xlabel(self.reader.get_arg_label(arg))
        ax.set_xticks(np.arange(start=1, stop=num_boxes + 1))
        ax.set_xticklabels(x_categories, rotation=45, ha="right")
        ax.set_ylabel("Average Duration (ns)")
        plt.ylim(bottom=0)
        fig.tight_layout()

        # save png and close current figure
        self.save_png("duration_boxplot", arg, bm_idx)
        plt.close()

    def plot_categorical_x(self, arg, y_value, bm_idx):
        x_values = self.reader.get_result(arg, bm_idx)
        y_values = self.reader.get_result(y_value, bm_idx)

        # actual plot
        x_pos = np.arange(len(x_values))
        plt.bar(x_pos, y_values)

        # set text of axes and x-ticks
        plt.xlabel(self.reader.get_arg_label(arg))
        plt.ylabel("Average Duration (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
        plt.xticks(x_pos, x_values, rotation=45, ha="right")
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_value, arg, bm_idx)
        plt.close()

    def plot_categorical_x_and_legend(self, perm, y_value, bm_idx):
        x_values = self.reader.get_result(perm[0], bm_idx)
        legend_values = self.reader.get_result(perm[1], bm_idx)
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)
        max_num_indices = max(len(v) for v in indices_per_legend_category.values())

        # create lexicographically ordered list of x-categories for correct y-value retrieval and x-ticks later
        x_categories = sorted(list(set(x_values)))

        # collect y-values for each legend category
        y_values_per_legend_category = defaultdict(list)
        for i, cat in enumerate(sorted(indices_per_legend_category.keys())):
            # init size of y-values dict
            y_values_per_legend_category[cat] = [0] * max_num_indices

            # fill y-values dict
            for idx in indices_per_legend_category[cat]:
                # ensure that dict is filled according to the order of categories on x-axis
                pos = x_categories.index(x_values[idx])
                y_values_per_legend_category[cat][pos] = self.reader.get_result(y_value, bm_idx)[idx]

        # actual plot
        x_pos = np.arange(len(x_categories))
        bar_width = 0.25
        fig, ax = plt.subplots()
        for j, cat in enumerate(y_values_per_legend_category.keys()):
            rects = ax.bar(x_pos + j * bar_width, y_values_per_legend_category[cat], bar_width, label=cat)

            # draw cross if y-value is null
            for rect in rects:
                if rect.get_height() == 0:
                    ax.annotate("X",
                                xy=(rect.get_x() + bar_width / 2, 0),
                                xytext=(rect.get_x() + bar_width / 2, 0),
                                ha="center",
                                va="baseline",
                                fontsize=18,
                                fontweight="bold",
                                color=plt.gca().containers[j].patches[0].get_facecolor())

        # set text of axes, x-ticks and legend title
        ax.set_xlabel(self.reader.get_arg_label(perm[0]))
        ax.set_ylabel("Average Duration (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
        bars_per_category = len(set(legend_values))
        x_ticks_pos = [x + (((bars_per_category / 2) - 0.5) * bar_width) for x in x_pos]
        ax.set_xticks(x_ticks_pos)
        ax.set_xticklabels(x_categories, rotation=45, ha="right")
        ax.legend(title=(self.reader.get_arg_label(perm[1]) + ":"), loc="upper left", bbox_to_anchor=(1, 1))
        fig.tight_layout()

        # save png and close current figure
        self.save_png(y_value, perm, bm_idx)
        plt.close()

    def plot_continuous_x(self, arg, y_value, bm_idx):
        x_values = self.reader.get_result(arg, bm_idx)
        y_values = self.reader.get_result(y_value, bm_idx)

        # actual plot
        plt.plot(x_values, y_values, "-o")

        # set text of axes
        plt.xlabel(self.reader.get_arg_label(arg))
        plt.ylabel("Average Duration (ns)" if y_value == "avg" else "Bandwidth (GB/s)")

        # adjust layout
        plt.xlim(left=0)
        plt.ylim(bottom=0)
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_value, arg, bm_idx)
        plt.close()

    def plot_continuous_x_and_legend(self, perm, y_value, bm_idx):
        x_values = self.reader.get_result(perm[0], bm_idx)
        legend_values = self.reader.get_result(perm[1], bm_idx)
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)

        # collect values for each legend category
        x_values_per_legend_category = defaultdict(list)
        y_values_per_legend_category = defaultdict(list)
        for cat in sorted(indices_per_legend_category.keys()):
            for idx in indices_per_legend_category[cat]:
                x_values_per_legend_category[cat].append(x_values[idx])
                y_values_per_legend_category[cat].append(self.reader.get_result(y_value, bm_idx)[idx])

        # actual plot
        for cat in x_values_per_legend_category.keys():
            plt.plot(x_values_per_legend_category[cat], y_values_per_legend_category[cat], "-o", label=cat)

        # set text of axes and legend title
        plt.xlabel(self.reader.get_arg_label(perm[0]))
        plt.ylabel("Average Duration (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
        plt.legend(title=(self.reader.get_arg_label(perm[1]) + ":"), loc="upper left", bbox_to_anchor=(1, 1))

        # adjust layout
        plt.xlim(left=0)
        plt.ylim(bottom=0)
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_value, perm, bm_idx)
        plt.close()
