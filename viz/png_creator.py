import glob
import sys
from itertools import permutations

from raw_json_plotter import RawJsonPlotter
from mat_json_plotter import MatrixJsonPlotter


class PngCreator:
    def __init__(self, results_dir, img_dir):
        self.results_dir = results_dir
        self.img_dir = img_dir

    def create_pngs_for_raw_jsons(self):
        # collect raw jsons of benchmarks
        raw_jsons = list()
        for path in glob.glob(self.results_dir + "raw/*.json"):
            raw_jsons.append(path)

        # create pngs
        if len(raw_jsons) > 0:
            for raw_json in raw_jsons:
                plotter = RawJsonPlotter(self.img_dir, raw_json)

                if plotter.reader.is_multithreaded():
                    plotter.latency_of_several_threads()
                else:
                    plotter.latency_of_same_thread()

                del plotter
    
    def create_pngs_for_matrix_json(self):
        # find json containing matrix arguments
        matrix_json = glob.glob(self.results_dir + "*.json")
        if len(matrix_json) != 1:
            sys.exit("Please provide exactly one json file (containing matrix_args field).")

        # init plotter
        plotter = MatrixJsonPlotter(self.img_dir, matrix_json[0])

        # iterate benchmark results
        for bm_idx in range(plotter.reader.get_num_benchmarks()):
            matrix_args = plotter.reader.get_result("matrix_args", bm_idx)

            # prevent visualization of more than three matrix arguments
            if len(matrix_args) > 3:
                name = plotter.reader.get_result("bm_name", bm_idx)
                print(f"Results of benchmark {name} cannot be visualized, because visualization of more than three "
                      f"matrix arguments is not supported as there would be too many plots.")
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
                    plotter.plot_categorical_x(matrix_args[0], "bandwidth_values", bm_idx)
                else:
                    pass
                    plotter.plot_continuous_x(matrix_args[0], "avg", bm_idx)
                    plotter.plot_continuous_x(matrix_args[0], "bandwidth_values", bm_idx)

        del plotter
