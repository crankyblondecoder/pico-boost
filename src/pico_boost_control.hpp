#ifndef PICO_BOOST_CONTROL_H
#define PICO_BOOST_CONTROL_H

// Boost control module.

/** Initialise boost control module. */
void boost_control_init();

/** Polling pass for boost control. */
void boost_control_poll();

/** Get whether the boost control solenoid is energised. */
bool boost_control_is_energised();

/** Get whether maximum boost has been reached. */
bool boost_control_max_boost_reached();

#endif