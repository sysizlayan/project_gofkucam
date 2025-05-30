#!/bin/bash

sudo apt install -y wget unzip cmake
cd extern
wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip

rm -rf libtorch-shared-with-deps-latest.zip

cd libtorch
touch CMakeLists.txt

