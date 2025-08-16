#ifndef PICO_BOOST_CONTROL_H
#define PICO_BOOST_CONTROL_H

// Boost control module.

/**
 * All boost control parameters in one place.
 */
struct boost_control_parameters {

	/** Maximum kPa. Scaled by 1000. */
	uint32_t max_kpa_scaled;

	/** Boost pressure below which the solenoid is de-energised. Scaled by 1000. */
	uint32_t de_energise_kpa_scaled;

	/** PID proportional constant. Scaled by 1000. */
	uint32_t pid_prop_const_scaled;

	/** PID integration constant. Scaled by 1000. */
	uint32_t pid_integ_const_scaled;

	/** PID derivative constant. Scaled by 1000. */
	uint32_t pid_deriv_const_scaled;

	/** Maximum duty cycle the solenoid can be set at. Scaled by 10. */
	uint32_t max_duty;

	/** zero point duty cycle of the solenoid. Scaled by 10. */
	uint32_t zero_point_duty;
};

/**
 * Get all boost control parameters.
 * @param params Populate this with current parameters.
 */
void boost_control_parameters_get(boost_control_parameters* params);

/**
 * Set all boost control parameters.
 * @param params Parameter data.
 */
void boost_control_parameters_set(boost_control_parameters* params);

/**
 * Set boost control parameters to defaults.
 * @param params Parameter data to set to defaults.
 */
void boost_control_parameters_populate_default(boost_control_parameters* params);

/** Initialise boost control module. */
void boost_control_init();

/** Polling pass for boost control. */
void boost_control_poll();

/** Get whether the boost control solenoid is energised. */
bool boost_control_is_energised();

/** Get whether maximum boost has been reached. */
bool boost_control_max_boost_reached();

/**
 * Get the current boost value, relative to std atm, as measured from the MAP sensor. In kPa, scaled by 1000.
 * Can be negative for below std atm.
 */
int boost_control_get_kpa_scaled();

/**
 * Get the current boost value, relative to std atm, as measured from the MAP sensor. In PSI, scaled by 10.
 * Can be negative for below std atm.
 */
int boost_control_get_psi_scaled();

/**
 * Get the maximum boost, relative to std atm. In kPa, scaled by 1000.
 */
unsigned boost_control_get_max_kpa_scaled();

/**
 * Set the maximum boost, relative to std atm. In kPa, scaled by 1000.
 */
void boost_control_set_max_kpa_scaled(unsigned maxKpaScaled);

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
 * Set the de-energise boost, relative to std atm. In kPa, scaled by 1000.
 */
void boost_control_set_de_energise_kpa_scaled(unsigned kpaScaled);

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
 * Set the PID proportional constant. Scaled by 1000.
 */
void boost_control_set_pid_prop_const_scaled(unsigned pidPropConstScaled);

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
 * Set the PID integration constant. Scaled by 1000.
 */
void boost_control_set_pid_integ_const_scaled(unsigned pidIntegConstScaled);

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
 * Set the PID derivative constant. Scaled by 1000.
 */
void boost_control_set_pid_deriv_const_scaled(unsigned pidDerivConstScaled);

/**
 * Alter the PID derivative constant by adjusting it by a delta.
 * @param pidDerivConstScaledDelta Delta, scaled by 1000.
 */
void boost_control_alter_pid_deriv_const_scaled(int pidDerivConstScaledDelta);

/**
 * Get maximum duty cycle the solenoid can be set at. Scaled by 10.
 */
unsigned boost_control_get_max_duty_scaled();

/**
 * Set maximum duty cycle the solenoid can be set at. Scaled by 10.
 */
void boost_control_set_max_duty_scaled(unsigned maxDuty);

/**
 * Alter maximum duty cycle the solenoid can be set at.
 * @param maxDutyDelta Delta, scaled by 10.
 */
void boost_control_alter_max_duty_scaled(int maxDutyDelta);

/**
 * Get zero point duty cycle of the solenoid. Scaled by 10.
 */
unsigned boost_control_get_zero_point_duty_scaled();

/**
 * Set zero point duty cycle of the solenoid. Scaled by 10.
 */
void boost_control_set_zero_point_duty_scaled(unsigned zeroPointDuty);

/**
 * Alter zero point duty cycle of the solenoid.
 * @param dutyDelta Delta, scaled by 10.
 */
void boost_control_alter_zero_point_duty_scaled(int dutyDelta);

/**
 * Get the current boost control solenoid duty cycle. Scaled by 1000.
 */
unsigned boost_control_get_current_duty_scaled();

/**
 * Run a test cycle with the solenoid valve.
 */
void boost_control_test_solenoid();

/**
 * Read the voltage supplied to the map sensor.
 */
double boost_map_read_supply_voltage();

/**
 * Read the current map sensor voltage.
 */
double boost_map_read_sensor_voltage();

#endif