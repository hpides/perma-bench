#!/usr/bin/env bash

if [ $# -ne 2 ]
then
    echo "Need to specify result and plot directory!"
    exit 1
fi

set -e
RESULT_DIR=$1
PLOT_DIR=$2

export PYTHONPATH="$PWD/scripts"

for script in scripts/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR} ${PLOT_DIR} > /dev/null
done

run_skript_dir() {
    local skript_dir=$1
    for script in scripts/$skript_dir/*.py
    do
        echo "Running $script..."
        python3 ${script} ${RESULT_DIR}/../$skript_dir ${PLOT_DIR} > /dev/null
    done
}

run_skript_dir hash_index
run_skript_dir pmem_tree_index
run_skript_dir speed
run_skript_dir dimms

open ${PLOT_DIR}/*.png
