#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ $# -lt 1 ]; then
    >&2 echo "The path to the results directory is missing."
    >&2 echo "Usage: ./visualize.sh ./path/to/results/dir"
    exit 1
fi

if [ ! -d "$DIR/venv" ]; then
    mkdir "$DIR/venv"
    python -m virtualenv "$DIR/venv"
    source "$DIR/venv/bin/activate"
    pip3 install -r "$DIR/requirements.txt"
fi
python "$DIR/viz/main.py" $1
