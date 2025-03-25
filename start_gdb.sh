#!/bin/bash

arm-none-eabi-gdb -ex "target extended-remote localhost:3333" -ex "monitor reset init" ./build/debug/pico_dash.elf
