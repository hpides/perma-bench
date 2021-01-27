#! ../viz/venv/bin/python
import os
import argparse

import user_interface as ui
from png_creator import PngCreator


def dir_path(path):
    if os.path.isdir(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"The path to the results directory is not valid.")


if __name__ == "__main__":
    # parse args + check for correctness and completeness of args
    parser = argparse.ArgumentParser()
    parser.add_argument("results_dir", type=dir_path, help="path to the results directory")
    parser.add_argument("--delete", action="store_true", help="delete already existing png and html files")
    args = parser.parse_args()

    # get directory paths of html and img folder
    root_dir = os.path.abspath(os.curdir)
    html_dir = os.path.join(root_dir, "viz/html")
    img_dir = os.path.join(args.results_dir, "img/")

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
