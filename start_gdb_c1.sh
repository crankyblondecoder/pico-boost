#!/bin/bash

echo "*** Note: gdb on first core must also be started. If either cores are halted, both will be. "
echo ""

arm-none-eabi-gdb -ex "target extended-remote localhost:3334" ./build/debug/pico_boost.elf
