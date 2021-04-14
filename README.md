# PerMA-Bench

A benchmarking suite and toolset to evaluate the performance of persistent memory access.

### Quick Start
PerMA-Bench has a predefined set of workloads to benchmark and requires very little configuration to run directly on
your system.
If you have [libnuma](https://github.com/numactl/numactl) installed in the
default locations, you can simply run the commands below.
Otherwise, you should briefly check out our [Build Options](#build-options) beforehand.

```shell script
$ git clone https://github.com/hpides/perma-bench.git
$ cd perma-bench
$ mkdir build && cd build
$ cmake ..
$ make -j
$ ./perma-bench --path /path/to/pmem/filesystem
```

### Build Options
In the following, we describe which build options you can provide for PerMA-Bench and how to configure them.

#### Using libnuma
In order to get NUMA-awareness in the benchmarks, you should have `libnuma` installed in the default location, e.g.,
via `apt install libnuma-dev` or `yum intall numactl-devel`.
If you have `libnuma` installed at a different location, you can specify `-DNUMA_INCLUDE_PATH` and `-DNUMA_LIBRARY_PATH`
to point to the respective headers and library in the `cmake` command.
If you do not have `libnuma` installed, PerMA-Bench will still work, just without NUMA-awareness of the threads.
In case you do not want to or can't install the development version, you can also manually enable NUMA-awareness by
using the `numactl` command line tool.

#### Building tests
By default, PerMA-Bench will not build the tests.
If you want to run the tests to make sure that everything was built correctly, you can specify `-DBUILD_TEST=ON` to
build the tests.
This is mainly relevant for development though.

### Running Benchmarks
TODO

### Configuring Benchmarks
TODO

### Visualization
PerMA-Bench offers a Python tool to visualize its results as PNGs and view them in a user interface. In order to do that, you have to build a virtual environment first. For this, the script `setup_viz.sh`, which requires Python 3 and Pip 3, must be sourced:
``` shell script
$ source setup_viz.sh
```

If a virtual environment has been built, the visualization can be started with the `main.py` script provided in the `viz` folder. When executing this script, it must be noted that it is called from the root folder of this project, i.e. `perma-bench`, and that it is passed the path to the result folder:
``` shell script
$ python3 viz/main.py path/to/results/dir/
```

The visualization produces PNG files that will be saved in a subdirectory of the result folder called `img/`. Additionally, you will find an `index.html` file as an entry point to the user interface and one HTML file for each benchmark in `viz/html/`. Note that each benchmark name must be unique and visualizations are only generated for benchmarks with a maximum of 3 matrix arguments.

With every run of the `main.py` script, you have the possibility to remove all already existing PNG and HTML files with the optional parameter `--delete`:
``` shell script
$ python3 viz/main.py path/to/results/dir/ --delete
```

For further information, open the help page of `main.py`:
``` shell script
$ python3 viz/main.py -h
```
