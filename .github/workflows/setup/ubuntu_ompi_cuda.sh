#!/usr/bin/env bash
#
# Copyright 2020 The HiPACE++ Community
#
# License: BSD-3-Clause-LBNL
# Authors: Axel Huebl

set -eu -o pipefail

sudo apt-get update

sudo apt-get install -y --no-install-recommends \
    build-essential     \
    g++-8               \
    libopenmpi-dev      \
    openmpi-bin         \
    nvidia-cuda-dev     \
    nvidia-cuda-toolkit
