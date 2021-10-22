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

for script in scripts/mixed/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR}/mixed ${PLOT_DIR} > /dev/null
done

for script in scripts/numa/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR}/numa ${PLOT_DIR} > /dev/null
done

for script in scripts/dimms/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR}/dimms ${PLOT_DIR} > /dev/null
done

for script in scripts/speed/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR}/speed ${PLOT_DIR} > /dev/null
done

for script in scripts/custom_join/*.py
do
    echo "Running $script..."
    python3 ${script} ${RESULT_DIR}/custom_join ${PLOT_DIR} > /dev/null
done

open ${PLOT_DIR}/*.png
