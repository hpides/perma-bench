#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -ge 1 ]; then
    if [ ! -d "$DIR/venv" ]; then
        mkdir "$DIR/venv"
        python -m virtualenv "$DIR/venv"
        source "$DIR/venv/bin/activate"
        pip3 install -r "$DIR/requirements.txt"
    fi
    python "$DIR/viz/main.py" $1
else
    echo "The path to the results directory is missing."
    echo "Usage: sh visualize.sh ./path/to/results/dir"
fi
