#!/bin/bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE:STRING=MinSizeRel ..
make
cd ..
