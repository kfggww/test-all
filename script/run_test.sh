#!/bin/bash

test_case=$1
project_dir=$(realpath $0 | xargs dirname | xargs dirname)

LD_LIBRARY_PATH=${project_dir}/install/lib ${project_dir}/install/bin/${test_case}