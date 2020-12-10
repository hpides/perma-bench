#! ../venv/bin/python
import os
import sys
import glob
import webbrowser

from raw_plotter import RawPlotter
import user_interface as ui
import utils


def create_pngs_for_raw_jsons(results_dir):
    # collect raw jsons of benchmarks
    raw_jsons = list()
    for path in glob.glob(results_dir + "raw/*.json"):
        raw_jsons.append(os.path.basename(path))

    # create pngs for raw jsons
    for raw_json in raw_jsons:
        utils.init(results_dir + "raw/" + raw_json)
        raw_plotter = RawPlotter(img_dir)

        if utils.is_multithreaded():
            raw_plotter.latency_of_several_threads()
        else:
            raw_plotter.latency_of_same_thread()

        del raw_plotter


def create_pngs_for_matrix_json(results_dir):
    # TODO
    print("Insert png creation of json containing matrices here.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("The path to the results directory is missing.\nUsage: python main.py ./path/to/results/dir")

    # create img folder
    root_dir = os.path.dirname(__file__)[:-3]
    results_dir = os.path.join(root_dir, str(sys.argv[1]))
    img_dir = os.path.join(results_dir, "img/")
    if not os.path.isdir(img_dir):
        os.makedirs(img_dir)

    # create pngs
    create_pngs_for_raw_jsons(results_dir)
    create_pngs_for_matrix_json(results_dir)

    # create and open user interface for pngs (at the moment: only for raw result pngs)
    ui.init(img_dir)
    webbrowser.open_new_tab("viz/html/index.html")
