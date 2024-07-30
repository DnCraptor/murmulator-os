#!/bin/bash
mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE:STRING=MinSizeRel -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE "-DCMAKE_C_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-gcc" "-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/arm-none-eabi-g++" --no-warn-unused-cli ..

make
cd ..
