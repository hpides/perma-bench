#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ ! -d "$DIR/viz/venv" ]; then
    mkdir "$DIR/viz/venv"
    python3 -m virtualenv "$DIR/viz/venv"
    source "$DIR/viz/venv/bin/activate"
    pip3 install -r "$DIR/viz/requirements.txt"
fi
