#ifndef PICO_BOOST_OPTIONS_H
#define PICO_BOOST_OPTIONS_H

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

/** Select display value options. */
enum SelectOption
{
	/** Current boost in the solenoid energised region. */
	CURRENT_BOOST,
	/** Maximum boost, in kPa. */
	BOOST_MAX_KPA,
	/** Boost value, in kPa, below which the wastegate solenoid is de-energised. */
	BOOST_DE_ENERGISE_KPA,
	/** Boost PID control, proportional constant. */
	BOOST_PID_PROP_CONST,
	/** Boost PID control, integration constant. */
	BOOST_PID_INTEG_CONST,
	/** Boost PID control, derivative constant. */
	BOOST_PID_DERIV_CONST,
	/** Boost control solenoid maximum duty cycle. In %. */
	BOOST_MAX_DUTY
};

// Boost options module.
// Controls display, including when in non-options mode.

/** Initialise the options. */
void boost_options_init();

/**
 * Boost options poll.
 */
void boost_options_poll();

#endif