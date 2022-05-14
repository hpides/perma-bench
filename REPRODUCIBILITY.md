# Reproducibility Guide for our PerMA-Bench Paper

This guide contains the instructions to reproduce the results of our Paper "PerMA-Bench: Benchmarking Persistent Memory Access".
The guide is split into three main parts: PerMA-Bench workloads, other system benchmarks, and visualization.

First off, all results and scripts for plotting are in the [`eval/`](https://github.com/hpides/perma-bench/tree/eval) branch.
After cloning the PerMA-Bench repository, you should check out that branch.

**Note:** To run *all* benchmarks, you will need physical access to your server and sudo rights.
The memory bus speed benchmarks require you to change the memory bus speed in the BIOS,
the number of DIMMs benchmarks require you to physically remove DIMMs from the server,
and the prefetcher workload requires you to disable the prefetcher.

The benchmarks were run on Ubuntu 20.04 with a 5.4.0 kernel.
Please look at Table 1 in the paper for more details on the individual servers.

## PerMA-Bench Workloads
### General Workloads
To reproduce all results that were run in PerMA-Bench, you can run the default benchmark suite in PerMA-Bench.
Please follow the instructions in the README on how to build PerMA-Bench and then execute the following command
```shell
$ ./perma-bench --path /path/to/pmem/filesystem -r results/
```
This will create a single JSON file in the `results/` directory with all runs in it.
These results cover a single server as shown in Figures 4, 5, 6, 7, 8, and 9, as well as parts of Figures 10, 11, 14, and 15.

### Memory Bus Speed Benchmarks
Here, you again need to run the default PerMA-Bench suite but with different memory speeds.
To change this, please check your server's BIOS documentation.
After rebooting, confirm that the speed was changed, e.g., via `dmidecode -t memory`.
Then, run the benchmark suite.

### Number of DIMMs
These runs are the default PerMA-Bench suite again, but with a varying number of DIMMs in the system.
Please check your system's documentation and population guide on how to correctly remove DIMMs.

Your system may vary, but you should generally be able to run:
```shell
$ umount /mnt/to/pmem
$ ndctl disable-namespace all
$ ndctl destroy-namespace all
$ ndctl disable-region all
$ ndctl list -NRi
```

The last command should not show you any valid namespaces anymore.
After removing the DIMMs, we ran `ipmctl create -f -goal persistentmemorytype=appdirect` in our BIOS to configure a new goal.
After rebooting, you need to configure PMem.
We used
```shell
$ ndctl create-namespace -r region0
$ mkfs.ext4 -F /dev/pmem0
$ mount -o dax /dev/pmem0 /mnt/perma/
$ chmod 1777 /mnt/perma/
$ mkdir /mnt/perma/perma
```

Then, run the benchmark suite with `-p /mnt/perma/perma`.

### Prefetcher
You do not need to run the entire suite for this benchmark.
We have a custom workload config YAML named [`prefetcher_reads.yaml`](https://github.com/hpides/perma-bench/blob/eval/scripts/prefetcher/prefetcher_reads.yaml) in the `scripts/prefetcher/` directory.
First, you can run PerMA-Bench with the prefetchers enabled
```shell
$ ./perma-bench -p /path/to/pmem -c /path/to/prefetcher_reads.yaml -r prefetcher_results
```
Now, you need to disable the prefetcher before running the prefetcher workload again.
On our servers, this was done via `sudo wrmsr --all 0x1a4 15`.
This sets all four prefetcher flags to 1 (15 = 1111).
Run the same command as above again.
**IMPORTANT:** Do not forget to enable the prefetchers once complete via `sudo wrmsr --all 0x1a4 0`!


## Other System Benchmarks
We also run benchmarks with a few other systems/data structures in our paper.
Here, we briefly describe how to reproduce the results, plot-by-plot.

### Dash in Figure 10
Clone Dash from the [public GitHub repository](https://github.com/baotonglu/dash).
Check out the version we used in the paper via `git checkout 824b94c`.
In `src/test_pmem.cpp` in line 26, you need to change the `pool_name` to run on your `/path/to/pmem`.
Build Dash as described in the author's README (standard CMake build).
Now, run the following commands (omit `numactl -N 0` if server has only one socket or change it to `-N X` if you are running on socket X):
```shell
$ numactl -N 0 ./test_pmem -op=insert -t=16 -p 100000000 -n 100000000
$ rm /path/to/pmem/pmem_ex.data  # Need to delete pool file named pmem_ex.data
$ numactl -N 0 ./test_pmem -op=pos -t=16 -p 100000000 -n 100000000
```
This will prefill 100 million records and run 100 million inserts/gets.
You can compare the results with our results in `results/hash_index/dash.txt`.

### FAST+FAIR in Figure 11
Clone FAST+FAIR from the [public GitHub repository](https://github.com/DICL/FAST_FAIR).
Check out the version we used in the paper via `git checkout 0f047e8`.
Change into the `concurrent_pmdk` directory.
Apply a patch we made to modify the benchmark.
The `.patch` file is located in `patches/ff.patch`.
You can apply it via `git apply /path/to/ff.patch`.
Build it via `make -j`.
Run the benchmark via (same numactl rules as for Dash above):
```shell
$ numactl -N 0 ./btree_concurrent -n 200000000 -t 16 -p /path/to/pmem/fftree
```
This will prefill 100 million records and then perform 100 million lookups, scans, and inserts.
You can compare the results with our results in `results/pmem_tree_index/fast_fair.txt`.

### FPTree eADR in Figure 12
We use pibench to benchmark FPTree, so the setup is a bit more complex.
Clone FPTree from the [pibench-ep2 public GitHub repository](https://github.com/sfu-dis/pibench-ep2).
Check out the version we used in the paper via `git checkout b67a1b3`.
Follow the authors' guide in the `FPTree/README.md` and the [original FPTree repository](https://github.com/sfu-dis/fptree) on the custom TBB build and on enabling TSX.
In `fptree.cpp`, modify `const char *path` to point to your pmem file location.
Build FPTree according to the guide.
Clone pibench from the [public GitHub repository](https://github.com/sfu-dis/pibench.git).
Check out the version we used in the paper via `git checkout 6df2044`.
Build pibench according to the authors' guide.
Run the benchmark via (same `numactl` rules apply again):
```shell
numactl -N 0 ./src/PiBench \
    --input /path/to/pibench-ep2/FPTree/build/src/libfptree_pibench_wrapper.so \
    --pcm=false -n 100000000 -p 100000000 \
    --pool_path /random/path \
    --pool_size 30000000000 \
    -t 32
```
In the version we used, the pool_path argument is ignored, as it is hard-coded in `fptree.cpp`.
Note that we run the experiment with 32 and 63 threads, so you need to run it twice.

Now, to modify FPTree for eADR, run the following `sed` command to replace all persists calls with `sfence`.
```shell
$ sed -i 's#pmemobj_persist#_mm_sfence();//pmemobj_persist#g' fptree.cpp
```
Build FPTree again and run the experiment.
Compare the results with ours in `scripts/eADR.py`.


### LB+Tree eADR in Figure 12
Clone LB+Tree from the [public GitHub repository](https://github.com/schencoding/lbtree).
Check out the version we used in the paper via `git checkout 92f7304`.
Make sure you have TSX enabled (see FPTree guide for details on how to enable it).
Build LB+Tree via `make`.
Go to the `keygen-8B` directory, call `make`, and add the following lines to `mygen.sh`:
```
./keygen 100000000 sort k100m
./keygen 100000000 random search100m
```
Now, generate data for LB+Tree via `./mygen.sh`.
This should create the files `k100m` and `search100m`.
To run the benchmark, run:
```shell
numactl -N 1 ./lbtree thread 32 mempool 10000 nvmpool /path/to/pmem/lbtree 20000 bulkload 100000000 keygen-8B/k100m 0.7 lookup 100000000 keygen-8B/search100m
```
You need to change the number of threads to 64 and run again.
To modify LB+Tree for eADR, modify line 59 in `common/nvm-common.h` and simply `return;` from the `clwb(...)` method instead of calling the inline assembly.
Call `make` again and run the experiment.
Compare the results with ours in `scripts/eADR.py`.


### Dash eADR in Figure 12
Use the Dash version as described above.
Run the same experiment as described above but with 32 and 64 threads.
Make sure the data file is deleted after each run!
Modify Dash the same way as FPTree via:
```shell
$ sed -i 's#pmemobj_persist#_mm_sfence();//pmemobj_persist#g' src/allocator.h
$ sed -i 's#pmemobj_persist#_mm_sfence();//pmemobj_persist#g' src/ex_finger.h
```
Call `make` again and run the experiment.
Compare the results with ours in `scripts/eADR.py`.


### Viper eADR in Figure 12
Clone Viper from the [public GitHub repository](https://github.com/hpides/viper).
Checkout the version we used in the paper via `git checkout 79ebf6e`.
In `benchmark/benchmark.hpp`, change the following lines in lines 111 - 114.
```cpp
static constexpr char VIPER_POOL_FILE[] = "/path/to/pmem/viper";
static constexpr char DB_PMEM_DIR[] = "/path/to/pmem/";
static constexpr char RESULT_FILE_DIR[] = "/tmp/results";
```
Then, apply the patch file in `patches/viper.patch`.
Build the benchmarks following the README.
Execute the benchmark via:
```shell
numactl -N 0 ./all_ops_bm
```
To modify Viper for eADR, simply remove the `_mm_clwb()` call in line 105 of `include/viper/viper.hpp`.
Build the benchmarks again and run the experiment.
Compare the results with ours in `scripts/eADR.py`.


## Visualization
### Re-creating our plots
We use matplotlib and Python3 to create our plots.
If you do not have this installed globally, you can set up a virtual environment, e.g., via `python3 -m virtualenv venv`.
Then, active it via `source venv/bin/activate` and install the dependencies via `pip install -r requirements.txt`.

You can create all plots in the paper as `.png` and `.svg` files by calling `./all_plots.sh results/pmem /tmp/plots` from the root of the eval branch.
This will create all plots in the paper (and some more) in `/tmp/plots`.

To create individual plots, you need to call the script for that plot manually.
Each script expects two arguments, the result directory and the output directory (same as in the command above).
Check the `all_plots.sh` script to see which script needs which result directory.
Most scripts need `results/pmem` but some need a different one, e.g., `prefetcher.py` needs the `prefetcher` directory results.

To create, e.g., the persist instruction plot (Figure 6 in paper), call the script and open the output file.
```shell
$ python3 scripts/persist_instruction.py results/pmem /tmp/plots
$ open /tmp/plots/persist_instruction.png
```

### Adding your Results to the Plots
We follow a fixed naming scheme for the result file, which you should also follow for the scripts to work out of the box.
The easiest way for you to do this is simply to replace one of the existing files with your results.
For example, if you also run the benchmarks on a server with 128 GB DIMMs, replace the `apache-256.json` file in `results/pmem`, so you can compare our results with yours side-by-side.

The scripts that require a different result directory than `results/pmem` either have different file names, e.g., `results/prefetcher` or symbolic links with the same names to `../pmem/<same-name>.json`.
So for most plots, you would only need to replace the file in `results/pmem`.
