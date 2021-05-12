#!/usr/bin/env bash

if [ $# -ne 2 ]
then
    echo "Need to specify result and plot directory!"
    exit 1
fi

set -e
RESULT_DIR=$1
PLOT_DIR=$2

for script in scripts/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR} ${PLOT_DIR} > /dev/null
done

open ${PLOT_DIR}/*.png
