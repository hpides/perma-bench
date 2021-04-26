#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ ! -d "$DIR/viz/venv" ]; then
    set -e
    virtualenv "$DIR/viz/venv"
    source "$DIR/viz/venv/bin/activate"
    pip3 install -r "$DIR/viz/requirements.txt"
else
    source "$DIR/viz/venv/bin/activate"
fi
