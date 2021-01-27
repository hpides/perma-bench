"""
This is the main module that has to be executed from the root directory, i.e. "perma-bench". It creates result PNGs and
displays them on an user interface.
"""

# ! ../viz/venv/bin/python

import argparse
import os
import sys
import user_interface as ui

from png_creator import PngCreator


def dir_path(path):
    """
    Checks if the given directory path is valid.

    :param path: directory path to the results folder
    :return: bool representing if path was valid
    """
    if os.path.isdir(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"The path to the results directory is not valid.")


if __name__ == "__main__":
    # check if python is running inside virtualenv or inside venv
    if not (getattr(sys, 'real_prefix', None) or (getattr(sys, 'base_prefix', sys.prefix)) != sys.prefix):
        sys.exit("Please run ./setup_viz.sh once to setup the visualization environment.")

    # parse args + check for correctness and completeness of args
    parser = argparse.ArgumentParser()
    parser.add_argument("results_dir", type=dir_path, help="path to the results directory")
    parser.add_argument("--delete", action="store_true", help="delete already existing png and html files")
    args = parser.parse_args()

    # get directory paths of html and img folder
    root_dir = os.path.abspath(os.getcwd())
    html_dir = os.path.join(root_dir, "viz/html")
    img_dir = os.path.join(os.path.abspath(args.results_dir), "img/")

    # delete already existing png and html files
    if args.delete:
        for file in os.listdir(html_dir):
            if file.endswith(".html"):
                os.remove(os.path.join(html_dir, file))
        for png in os.listdir(img_dir):
            os.remove(os.path.join(img_dir, png))

    # create img folder
    if not os.path.isdir(img_dir):
        os.makedirs(img_dir)

    # create pngs
    png_creator = PngCreator(args.results_dir, img_dir)
    png_creator.process_raw_jsons()
    png_creator.process_matrix_jsons()

    # create user interface for pngs
    ui.init(img_dir)
    print(f"The visualization is finished. The result png files are located in {img_dir} and can be viewed in a "
          f"browser with \"<browser-name> {html_dir}/index.html\"")
