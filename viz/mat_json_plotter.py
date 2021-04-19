import matplotlib.pyplot as plt
import numpy as np

from collections import defaultdict
from mat_json_reader import MatrixJsonReader


class MatrixJsonPlotter:
    """
        This class provides methods to create PNGs for benchmarks with non-raw result JSONs.
    """

    def __init__(self, img_dir, path):
        self.img_dir = img_dir
        self.reader = MatrixJsonReader(path)

    """
        helper functions:
    """

    def get_indices_of_legend_categories(self, legend_values):
        indices_per_legend_category = defaultdict(list)
        for idx, val in enumerate(legend_values):
            indices_per_legend_category[val].append(idx)

        return indices_per_legend_category

    def save_png(self, y_value, args, bm_idx):
        bm_name = self.reader.get_result("bm_name", bm_idx)

        if y_value == "avg":
            y_value = "average_duration"
        elif y_value == "bandwidth_values":
            y_value = "bandwidth"

        if isinstance(args, tuple):
            plt.savefig(f"{self.img_dir}{bm_name}-{args[0]}-{args[1]}-{y_value}.png")
        else:
            plt.savefig(f"{self.img_dir}{bm_name}-{args}-{y_value}.png")

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
        ax.set_ylabel("Duration (ns)")
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
        plt.ylabel("Average Latency (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
        plt.xticks(x_pos, x_values, rotation=45, ha="right")
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_value, arg, bm_idx)
        plt.close()

    def plot_categorical_x_and_legend(self, perm, y_value, bm_idx):
        x_values = self.reader.get_result(perm[0], bm_idx)
        legend_values = self.reader.get_result(perm[1], bm_idx)
        title_values = self.reader.get_result(perm[2], bm_idx) if len(perm) == 3 else [None] * len(x_values)
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)
        max_num_indices = max(len(v) for v in indices_per_legend_category.values())
        bm_results = self.reader.get_result(y_value, bm_idx)

        # create lexicographically ordered list of x-categories for correct y-value retrieval and x-ticks later
        x_categories = sorted(set(x_values))
        num_categories = len(x_categories)

        # collect y-values for each legend category
        y_values_per_legend_category = defaultdict(lambda: defaultdict(list))
        for cat, indices in indices_per_legend_category.items():
            for idx in indices:
                title_val = title_values[idx]
                y_values_per_legend_category[title_val][cat].append(bm_results[idx])

        # actual plot
        plot_titles = sorted(set(title_values))
        num_plots = len(plot_titles)
        fig, axes = plt.subplots(1, num_plots, figsize=(num_plots * 4, 3))
        if num_plots == 1:
            axes = [axes]

        x_pos = np.arange(num_categories)
        bar_width = 0.25

        for i, title_val in enumerate(plot_titles):
            ax = axes[i]
            y_vals = y_values_per_legend_category[title_val]
            for offset, cat in enumerate(y_vals.keys()):
                rects = ax.bar(x_pos + offset * bar_width, y_vals[cat], bar_width, label=cat)

                # draw cross if y-value is null
                for rect in rects:
                    if rect.get_height() == 0:
                        ax.annotate("X", xy=(rect.get_x() + bar_width / 2, 0), xytext=(rect.get_x() + bar_width / 2, 0),
                                    ha="center", va="baseline", fontsize=18, fontweight="bold",
                                    color=plt.gca().containers[i].patches[0].get_facecolor())

            # set text of axes, x-ticks and legend title
            ax.set_xlabel(self.reader.get_arg_label(perm[0]))
            ax.set_ylabel("Average Latency (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
            bars_per_category = len(set(legend_values))
            x_ticks_pos = [x + (((bars_per_category / 2) - 0.5) * bar_width) for x in x_pos]
            ax.set_xticks(x_ticks_pos)
            ax.set_xticklabels(x_categories, rotation=0, ha="center")

            if title_val is not None:
                ax.set_title(f"{self.reader.get_arg_label(perm[2])} = {title_val}")

        plt.legend(title=(self.reader.get_arg_label(perm[1]) + ":"), loc="upper left", bbox_to_anchor=(1, 1))
        plt.tight_layout()

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
        plt.ylabel("Average Latency (ns)" if y_value == "avg" else "Bandwidth (GB/s)")

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
        title_values = self.reader.get_result(perm[2], bm_idx) if len(perm) == 3 else [None] * len(x_values)
        indices_per_legend_category = self.get_indices_of_legend_categories(legend_values)
        bm_results = self.reader.get_result(y_value, bm_idx)

        # collect values for each legend category
        x_values_per_legend_category = defaultdict(lambda: defaultdict(list))
        y_values_per_legend_category = defaultdict(lambda: defaultdict(list))
        for cat in sorted(indices_per_legend_category.keys()):
            for idx in indices_per_legend_category[cat]:
                title_val = title_values[idx]
                x_values_per_legend_category[title_val][cat].append(x_values[idx])
                y_values_per_legend_category[title_val][cat].append(bm_results[idx])

        # actual plot
        num_plots = len(x_values_per_legend_category.keys())
        fig, axes = plt.subplots(1, num_plots, figsize=(num_plots * 4, 3))
        if num_plots == 1:
            axes = [axes]

        for i, title_val in enumerate(x_values_per_legend_category.keys()):
            ax = axes[i]
            x_vals = x_values_per_legend_category[title_val]
            y_vals = y_values_per_legend_category[title_val]
            for cat in x_vals.keys():
                ax.plot(x_vals[cat], y_vals[cat], "-o", label=cat)

            ax.set_ylabel("Average Latency (ns)" if y_value == "avg" else "Bandwidth (GB/s)")
            ax.set_xlabel(self.reader.get_arg_label(perm[0]))
            x_ticks = list(x_vals.values())[0]
            ax.set_xticks(x_ticks)
            ax.set_xticklabels(x_ticks)
            ax.set_xlim(left=0)
            ax.set_ylim(bottom=0)
            if title_val is not None:
                ax.set_title(f"{self.reader.get_arg_label(perm[2])} = {title_val}")

        # set text of axes and legend title
        plt.legend(title=(self.reader.get_arg_label(perm[1]) + ":"), loc="upper left", bbox_to_anchor=(1, 1))
        plt.tight_layout()

        # save png and close current figure
        self.save_png(y_value, perm, bm_idx)
        plt.close()
