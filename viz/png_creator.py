import glob
import sys
import warnings

from itertools import permutations
from mat_json_plotter import MatrixJsonPlotter
from raw_json_plotter import RawJsonPlotter


class PngCreator:
    def __init__(self, results_dir, img_dir):
        self.results_dir = results_dir
        self.img_dir = img_dir

    def create_pngs_for_raw_jsons(self):
        # collect raw jsons of benchmarks
        raw_jsons = list()
        for path in glob.glob(self.results_dir + "/raw/*.json"):
            raw_jsons.append(path)

        # create pngs
        if len(raw_jsons) > 0:
            for raw_json in raw_jsons:
                plotter = RawJsonPlotter(self.img_dir, raw_json)

                if plotter.reader.is_multithreaded():
                    plotter.latency_of_several_threads()
                else:
                    plotter.latency_of_same_thread()

    def create_pngs_for_matrix_json(self):
        # collect jsons containing matrix arguments
        matrix_jsons = list()
        for path in glob.glob(self.results_dir + "/*.json"):
            matrix_jsons.append(path)

        if len(matrix_jsons) < 1:
            sys.exit(f"Visualization cannot be started until at least one JSON file is provided in {self.results_dir}.")

        for mat_json in matrix_jsons:
            plotter = MatrixJsonPlotter(self.img_dir, mat_json)
            bm_names = plotter.reader.get_bm_names()
            indices_to_skip = list()

            # iterate benchmark results
            for bm_idx, bm_name in enumerate(bm_names):

                # forbid visualization for benchmarks whose names occur multiple times
                if bm_idx in indices_to_skip:
                    continue

                if bm_names.count(bm_name) > 1:
                    warnings.warn(f"Benchmark names have to be unique. Since this does not apply to \"{bm_name}\", no "
                                  f"visualization was generated for this benchmark.")
                    [indices_to_skip.append(idx) for idx, name in enumerate(bm_names) if name == bm_name]
                    continue

                # only execute visualization for benchmark if matrix arguments exist
                if plotter.reader.get_result("matrix_args", bm_idx):
                    matrix_args = plotter.reader.get_result("matrix_args", bm_idx)

                    # prevent visualization of more than three matrix arguments
                    if len(matrix_args) > 3:
                        warnings.warn(f"Results of benchmark \"{bm_name}\" cannot be visualized, because "
                                      f"visualization of more than three matrix arguments is not supported as there "
                                      f"would be too many plots.")
                        continue

                    # create pngs for matrix with two or three dimensions
                    elif 2 <= len(matrix_args) <= 3:
                        perms = list(permutations(matrix_args, 2))

                        for perm in perms:
                            if perm[0] in plotter.reader.get_categorical_args():
                                plotter.plot_categorical_x_and_legend(perm, "avg", bm_idx)
                                plotter.plot_categorical_x_and_legend(perm, "bandwidth_values", bm_idx)
                            else:
                                plotter.plot_continuous_x_and_legend(perm, "avg", bm_idx)
                                plotter.plot_continuous_x_and_legend(perm, "bandwidth_values", bm_idx)

                    # create pngs for matrix with one dimension
                    else:
                        if matrix_args[0] in plotter.reader.get_categorical_args():
                            plotter.plot_categorical_x(matrix_args[0], "avg", bm_idx)
                            plotter.plot_duration_boxes(matrix_args[0], bm_idx)
                            plotter.plot_categorical_x(matrix_args[0], "bandwidth_values", bm_idx)
                        else:
                            plotter.plot_continuous_x(matrix_args[0], "avg", bm_idx)
                            plotter.plot_continuous_x(matrix_args[0], "bandwidth_values", bm_idx)

                else:
                    warnings.warn(f"Results of benchmark \"{bm_name}\" cannot be visualized as no matrix arguments are "
                                  f"defined.")
