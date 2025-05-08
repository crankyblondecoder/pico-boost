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

/** Get the current boost value, relative to std atm, as measured from the MAP sensor. In kPa, scaled by 1000. */
unsigned boost_control_get_kpa_scaled();

/**
 * Get the maximum boost, relative to std atm. In kPa, scaled by 1000.
 */
unsigned boost_control_get_max_kpa_scaled();

/**
 * Alter the maximum boost, relative to std atm, by adjusting it by a delta.
 * @param maxBoostKpaScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_max_kpa_scaled(int maxBoostKpaScaledDelta);

/**
 * Get the de-energise boost, relative to std atm. In kPa, scaled by 1000.
 */
unsigned boost_control_get_de_energise_kpa_scaled();

/**
 * Alter the de-energise boost, relative to std atm, by adjusting it by a delta.
 * @param deEnergiseKpaScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_de_energise_kpa_scaled(int deEnergiseKpaScaledDelta);

/**
 * Get the PID proportional constant. Scaled by 1000.
 */
unsigned boost_control_get_pid_prop_const_scaled();

/**
 * Alter the PID proportional constant by adjusting it by a delta.
 * @param pidPropConstScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_pid_prop_const_scaled(int pidPropConstScaledDelta);

/**
 * Get the PID integration constant. Scaled by 1000.
 */
unsigned boost_control_get_pid_integ_const_scaled();

/**
 * Alter the PID integration constant by adjusting it by a delta.
 * @param pidIntegConstScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_pid_integ_const_scaled(int pidIntegConstScaledDelta);

/**
 * Get the PID derivative constant. Scaled by 1000.
 */
unsigned boost_control_get_pid_deriv_const_scaled();

/**
 * Alter the PID derivative constant by adjusting it by a delta.
 * @param pidDerivConstScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_pid_deriv_const_scaled(int pidDerivConstScaledDelta);

/**
 * Get maximum duty cycle the solenoid can be set at.
 */
unsigned boost_control_get_max_duty();

/**
 * Alter maximum duty cycle the solenoid can be set at.
 * @param maxDutyDelta Delta, not scaled.
 */
void boost_control_alter_max_duty(int maxDutyDelta);

#endif