# PerMA-Bench

A benchmarking suite and toolset to evaluate the performance of persistent memory access.

### Quick Start
PerMA-Bench has a predefined set of workloads to benchmark and requires very little configuration to run directly on
your system.
If you have the development version of [libnuma](https://github.com/numactl/numactl) (PerMA-Bench requires the headers) 
installed in the default locations, e.g., via `apt install libnuma-dev`, you can simply run the commands below.
Otherwise, you should briefly check out our [Build Options](#build-options) beforehand.

```shell script
$ git clone https://github.com/hpides/perma-bench.git
$ cd perma-bench
$ mkdir build && cd build
$ cmake ..
$ make -j
$ ./perma-bench --path /path/to/pmem/filesystem
```

This will create a `results` directory containing a JSON file with all benchmark resutls in it.

### Build Options
In the following, we describe which build options you can provide for PerMA-Bench and how to configure them.

#### Using libnuma
In order to get NUMA-awareness in the benchmarks, you should have `libnuma` installed in the default location, e.g.,
via `apt install libnuma-dev` or `yum intall numactl-devel`.
If you have `libnuma` installed at a different location, you can specify `-DNUMA_INCLUDE_PATH` and `-DNUMA_LIBRARY_PATH`
to point to the respective headers and library in the `cmake` command.
If you do not have `libnuma` installed, PerMA-Bench will still work, just without NUMA-awareness of the threads.
In case you do not want to or can't install the development version, you can also manually enable NUMA-awareness by
using the `numactl` command line tool and disabling NUMA-awareness in PerMA-Bench via the `--no-numa` flag.
You should pin the application to the nodes that are close to your mounted persistent memory filesystem for the best performance e.g., like this:

```shel script
$ numactl -N 0,1 ./perma-bench --path /path/to/pmem/filesystem --no-numa
```

#### Building tests
By default, PerMA-Bench will not build the tests.
If you want to run the tests to make sure that everything was built correctly, you can specify `-DBUILD_TEST=ON` to
build the tests.
This is mainly relevant for development though.

### Configuring Benchmarks
TODO

### Running Custom Benchmarks
TODO

### Visualization
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
