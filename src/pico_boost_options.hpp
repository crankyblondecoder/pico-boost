#ifndef PICO_BOOST_OPTIONS_H
#define PICO_BOOST_OPTIONS_H

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

// Boost options module.
// Controls display, including when in non-options mode.

/** Initialise the options. */
void boost_options_init();

/**
 * Boost options poll.
 */
void boost_options_poll();

#endif