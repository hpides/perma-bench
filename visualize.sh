#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
RESULT_DIR="$1"
DELETE_BOOL="$2"

if [ $# -lt 1 ]; then
    >&2 echo "No arguments are provided."
    >&2 echo "Usage: ./visualize.sh /path/to/results/dir [--delete]"
    exit 1
elif [ ! -d $RESULT_DIR ]; then
    >&2 echo "The path to the results directory is not valid or missing."
    >&2 echo "Usage: ./visualize.sh /path/to/results/dir [--delete]"
    exit 1
fi

if [ ! -d "$DIR/viz/venv" ]; then
    mkdir "$DIR/viz/venv"
    python3 -m virtualenv "$DIR/viz/venv"
    source "$DIR/viz/venv/bin/activate"
    pip3 install -r "$DIR/viz/requirements.txt"
fi

python3 "$DIR/viz/main.py" $RESULT_DIR $DELETE_BOOL
