#!/usr/bin/env bash

if [ $# -eq 0 ]
then
    echo "Need to specify plot output directory!"
    exit 1
fi

set -e
PLOT_DIR=$1

for script in scripts/*.py
do
    echo "Running $script..."
    python3 ${script} results ${PLOT_DIR} > /dev/null
done

open ${PLOT_DIR}/*.png
