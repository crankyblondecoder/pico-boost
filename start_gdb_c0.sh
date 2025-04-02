#!/bin/bash

echo "Warning: 'monitor reset init' will be executed"
echo ""

arm-none-eabi-gdb -ex "target extended-remote localhost:3333" -ex "monitor reset init" ./build/debug/pico_boost.elf
