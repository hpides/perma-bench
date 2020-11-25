# PerMA-Bench

A benchmarking suite and toolset to evaluate the performance of persistent memory access.

### Quick Start
PerMA-Bench has a predefined set of workloads to benchmark and requires very little configuration to run directly on 
your system.
If you have [libpmem](https://github.com/pmem/pmdk) and [libnuma](https://github.com/numactl/numactl) installed in the 
default locations, you can simply run the commands below.
Otherwise, you should briefly check out our [Build Options](#build-options) beforehand.

```shell script
$ git clone https://github.com/hpides/perma-bench.git
$ cd perma-bench
$ mkdir build && cd build
$ cmake ..
$ make -j
$ ./perma-bench /path/to/pmem/filesystem 
``` 

### Build Options
In the following, we describe which build options you can provide for PerMA-Bench and how to configure them.

#### Using libpmem
The default build options specify that PMDK's `libpmem` is provided by the system at the default locations.
If you have `libpmem` installed at a different location, you can specify `-DPMDK_INCLUDE_PATH` and `-DPMDK_LIBRARY_PATH` 
to point to the respective headers and library in the `cmake` command.
If you wish to ignore you system's `libpmem` or it is not installed, you can add `-DBUILD_PMDK=ON`, which will download 
PMDK's latest release and install it from source. 
In that case, you need to have PMDK's dependencies installed.
Please check the current dependencies in the [PMDK README](https://github.com/pmem/pmdk/#dependencies) specific to your system.

#### Using libnuma
In order to get NUMA-awareness in the benchmarks, you should have `libnuma` installed in the default location, e.g., 
via `apt install libnuma-dev` or `yum intall numactl-devel`.
If you have `libnuma` installed at a different location, you can specify `-DNUMA_INCLUDE_PATH` and `-DNUMA_LIBRARY_PATH` 
to point to the respective headers and library in the `cmake` command.
If you do not have `libnuma` installed, PerMA-Bench will still work, just without NUMA-awareness of the threads.
In case you do not want to or can't install the development version, you can also manually enable NUMA-awareness by 
using the `numactl` command line tool. 

#### Testing
By default, PerMA-Bench will not build and run the tests.
If you want to run the tests to make sure that everything was built correctly, you can specify `-DBUILD_TEST=ON` to
build the tests.
This is mainly relevant for development though.

### Configuring Benchmarks
    
