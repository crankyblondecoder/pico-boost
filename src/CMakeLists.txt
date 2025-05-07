cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_boost C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Add extra source files here.
add_executable(pico_boost
	AdcReader.cpp
	BoschMap_0261230119.cpp
	Eeprom.cpp
	Eeprom_24CS256.cpp
	pico_boost.cpp
	pico_boost_options.cpp
	pico_boost_control.cpp
	PicoAdcReader.cpp
	PicoSwitch.cpp
	TM1637_pico.cpp)

# Set to 1 to enable.
pico_enable_stdio_usb(pico_boost 1)
pico_enable_stdio_uart(pico_boost 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(pico_boost)

target_link_libraries(pico_boost
	pico_stdlib
	pico_multicore
	pico_time
	pico_platform
	hardware_adc
	hardware_clocks
	hardware_gpio
	hardware_i2c)
