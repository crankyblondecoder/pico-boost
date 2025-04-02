#!/bin/bash

arm-none-eabi-gdb -ex "target extended-remote localhost:3334" ./build/debug/pico_boost.elf
