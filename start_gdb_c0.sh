#!/bin/bash

echo "*** Note: gdb on second core must also be started. If either cores are halted, both will be."
echo ""

arm-none-eabi-gdb -ex "target extended-remote localhost:3333" -ex "monitor reset init" ./build/debug/pico_boost.elf
