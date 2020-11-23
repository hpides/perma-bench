#!/usr/bin/env bash

############################################## DEFINE AND SET VARIABLES ################################################
VISUALIZE=0
DEBUG=0
PMEM_DIR=/mnt/pmem
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
        shift # Remove --debug from processing
        ;;
        --server)
        # shellcheck disable=SC2034
        PMEM_DIR=/mnt/nvram-nvmbm
        shift # Remove --server from processing
        ;;
    esac
done

############################################# BUILD AND RUN CMAKE PROJECT ##############################################
if [ "$DEBUG" == 1 ]; then
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/opt/rh/devtoolset-9/root/usr/bin/make -DCMAKE_C_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/gcc -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++ -G "CodeBlocks - Unix Makefiles"
  "$DIR/../cmake-build-debug/src/perma-bench" PMEM_DIR
else
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=/opt/rh/devtoolset-9/root/usr/bin/make -DCMAKE_C_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/gcc -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++ -G "CodeBlocks - Unix Makefiles"
  "$DIR/../cmake-build-release/src/perma-bench" PMEM_DIR
fi

################################# SETUP VIRTUAL ENVIRONMENT AND EXECUTE VISUALIZATION ##################################
if [ "$VISUALIZE" == 1 ]; then
  if [ ! -d "$DIR/../venv" ]; then
    mkdir "$DIR/../venv"
  fi
  python3 -m virtualenv "$DIR/../venv"

  # shellcheck disable=SC1090
  source "$DIR/../venv/bin/activate"
  pip3 install -r "$DIR/../requirements.txt"

  # shellcheck disable=SC2164
  python3 "$DIR/../visualization/main.py"
fi
