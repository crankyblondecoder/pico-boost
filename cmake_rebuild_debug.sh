#!/bin/bash

# Full cmake rebuild.

# See https://forums.raspberrypi.com/viewtopic.php?t=315354 as to why using PICO_DEOPTIMIZED_DEBUG=1 might be bad.

cd ./build/debug

rm -rf *

# This is absolutely vital otherwise cmake _doesn't_ use the cross compiler!
export PICO_SDK_PATH=../../pico-sdk/

cmake -DCMAKE_BUILD_TYPE=Debug -DPICO_DEOPTIMIZED_DEBUG=1 ../../src/
