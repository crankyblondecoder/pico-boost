#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

/**
 * Process Pico Boost options.
 * @param upSelectButton Up/Select button.
 * @param downButton down button.
 * @param display Display to use.
 * @returns True if options output to display. ie It is currently controlling the display.
 */
bool process_options(PicoSwitch& upSelectButton, PicoSwitch& downButton, TM1637Display& display)
{
	// TODO ...
}