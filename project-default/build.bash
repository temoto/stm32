#!/bin/bash
set -e
cd $(dirname $0) ; cd ..
srcs=(
  main.c
)
source build-helper.bash
main $*
