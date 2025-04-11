#ifndef PICO_BOOST_OPTIONS_H
#define PICO_BOOST_OPTIONS_H

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

struct option
{
	/** Displayed character bitmap for option. */
	uint8_t displayedCharBitmap;
};

/** Initialise the options. */
void init_options();

/**
 * Process Pico Boost options.
 * @param upSelectButton Up/Select button.
 * @param downButton down button.
 * @param display Display to use.
 * @returns True if options output to display. ie It is currently controlling the display.
 */
bool process_options(PicoSwitch& upSelectButton, PicoSwitch& downButton, TM1637Display& display);

#endif