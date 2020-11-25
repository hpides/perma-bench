#! ../venv/bin/python
import matplotlib.pyplot as plt
import os
import sys


def main():
    # create img folder
    img_dir = os.path.join(str(sys.argv[1]), "img/")

    if not os.path.isdir(img_dir):
        os.makedirs(img_dir)

    # test plot
    plt.plot([1, 2, 3, 4])
    plt.savefig(img_dir + "/test.png")
    plt.close()
    print("Visualization was successful.")


if __name__ == "__main__":
    main()
