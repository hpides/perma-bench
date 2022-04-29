LD_PRELOAD=/scratch/pmem/pmdk/lib/libpmemobj.so numactl -N 1 ./src/PiBench \
	--input /scratch/lawrence.benson/fptree/build/src/libfptree_pibench_wrapper.so \
	--pcm=false -n 100000000 -p 100000000 -t 20 \
	--pool_path /mnt/pmem2/fptree \
	--pool_size 30000000000

