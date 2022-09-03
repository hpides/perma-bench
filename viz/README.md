## Visualizing Results

PerMA-Bench offers a Python tool to visualize its results as PNGs and view them in in the browser as HTML files.
In order to do that, you have to create a Python virtual environment first.
The script `setup_viz.sh`, which requires Python3, Pip3 and python3-venv, must be sourced:
``` shell script
$ source setup_viz.sh
```

If a virtual environment has been created, the visualization can be started with the `main.py` script provided in the `viz` folder.
The script must be called from the root folder of this project, i.e. `perma-bench`.
The basic version of the script requires a path the the result directory (or single file) of the benchmark run and an output directory.
If you are creating the plots on a server, you can add `--zip` to create a zip archive of the output directoty, which can then easily be downloaded onto your local machine for inspection.
``` shell script
$ python3 viz/main.py /path/to/results /path/to/output [--zip]
```

The visualization produces PNG files a subdirectory of the output directory folder called `img/`.
Additionally, you will find an `index.html` file as an entry point to the HTML files for each benchmark in the output directory.
Note that each benchmark name must be unique and visualizations are only generated for benchmarks with a maximum of 3 matrix arguments.

For further information, open the help page of `main.py`:
``` shell script
$ python3 viz/main.py -h
```
