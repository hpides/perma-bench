#!/usr/bin/env bash

find ../src -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i