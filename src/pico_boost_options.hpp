#ifndef PICO_BOOST_OPTIONS_H
#define PICO_BOOST_OPTIONS_H

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

/** The GPIO pin that is set high to indicate that boost options initiated testing is active. */
#define BOOST_OPTIONS_TEST_ACTIVE_GPIO 3

/** Select display value options. */
enum SelectOption
{
	/** Current boost as read by map sensor. PSI Output. */
	CURRENT_BOOST_PSI,

	/** Current boost as read by map sensor. KPA Output. */
	CURRENT_BOOST_KPA,

	/**
	 * Current duty cycle being appied to solenoid valve.
	 * @note Always keep this second. If it has to change, alter the edit mode entry if statement.
	 */
	CURRENT_DUTY,

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
	BOOST_MAX_DUTY,

	/** Boost control solenoid zero point duty cycle. In %. */
	BOOST_ZERO_POINT_DUTY,

	/** Maximum brightness of display. */
	DISPLAY_MAX_BRIGHTNESS,

	/** Minimum brightness of display. (ie "Headlights on")*/
	DISPLAY_MIN_BRIGHTNESS,

	/** Not an option, just used to indicate enum length. */
	SELECT_OPTION_LAST
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