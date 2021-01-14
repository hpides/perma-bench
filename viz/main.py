#! ../viz/venv/bin/python
import os
import sys

import user_interface as ui
from png_creator import PngCreator


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("The path to the results directory is missing.\nUsage: python main.py /path/to/results/dir")

    # create img folder
    root_dir = os.path.abspath(os.curdir)
    results_dir = os.path.join(root_dir, str(sys.argv[1]))
    img_dir = os.path.join(results_dir, "img/")
    if not os.path.isdir(img_dir):
        os.makedirs(img_dir)

    # create pngs
    png_creator = PngCreator(results_dir, img_dir)
    png_creator.create_pngs_for_raw_jsons()
    png_creator.create_pngs_for_matrix_json()

    # create user interface for pngs
    ui.init(img_dir)
    print(f"The visualization is finished. The result png files are located in {img_dir} and can be viewed "
          f"in a browser with \"<browser-name> {root_dir}/viz/html/index.html\"")
