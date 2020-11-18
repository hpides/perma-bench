#!/usr/bin/env bash

VISUALIZE=0
DEBUG=0
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

for arg in "$@"
do
    case $arg in
        --visualize)
        VISUALIZE=1
        shift # Remove --visualize from processing
        ;;
        --debug)
        # shellcheck disable=SC2034
        DEBUG=1
        shift # Remove --visualize from processing
        ;;
    esac
done


# shellcheck disable=SC2164
cd "$DIR/.."
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/opt/rh/devtoolset-9/root/usr/bin/make -DCMAKE_C_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/gcc -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++ -G "CodeBlocks - Unix Makefiles"

if [ "$DEBUG" == 1 ]; then
  ./cmake-build-debug/src/perma-bench
else
  ./cmake-build-release/src/perma-bench
fi

if [ "$VISUALIZE" == 1 ]; then
  mkdir "$DIR/../venv"
  python3 -m virtualenv "$DIR/../venv"
  pip3 install --user -r "$DIR/../requirements.txt"
  # shellcheck disable=SC1090
  source "$DIR/../venv/bin/activate"

  # shellcheck disable=SC2164
  python3 "$DIR/../visualization/main.py"
fi
