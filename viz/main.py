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
        sys.exit("Please run `source setup_viz.sh` once to setup the visualization environment.")

    # parse args + check for correctness and completeness of args
    parser = argparse.ArgumentParser()
    parser.add_argument("results_dir", type=dir_path, help="path to the results directory")
    parser.add_argument("output_dir", help="path to the output directory")
    parser.add_argument("--delete", action="store_true", help="delete already existing PNG and HTML files")
    args = parser.parse_args()

    # get directory paths of html and img folder
    root_dir = os.path.abspath(os.getcwd())
    output_dir = os.path.abspath(args.output_dir)
    img_dir = os.path.join(output_dir, "img/")

    try:
        os.makedirs(output_dir)
    except FileExistsError:
        if os.listdir(output_dir):
            sys.exit("Cannot write results to non-empty output directory.")

    # delete already existing png and html files
    if args.delete:
        if os.path.exists(output_dir):
            for file in os.listdir(output_dir):
                if file.endswith(".html"):
                    os.remove(os.path.join(output_dir, file))
        if os.path.exists(img_dir):
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
    print("Generating HTML pages")
    ui.init(output_dir, img_dir)
    print(f"\nThe visualization is finished. The result PNG files are located in {img_dir} and can be viewed in a "
          f"browser with\n\t$ <browser-name> {output_dir}/index.html")
