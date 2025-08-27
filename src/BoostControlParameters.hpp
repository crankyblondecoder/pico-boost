#ifndef BOOST_CONTROL_PARAMETERS_H
#define BOOST_CONTROL_PARAMETERS_H

#include <cstdint>

/**
 * All boost control parameters in one place.
 */
struct BoostControlParameters {

	/** Maximum kPa. Scaled by 1000. */
	uint32_t maxKpaScaled;

	/** Boost pressure below which the solenoid is de-energised. Scaled by 1000. */
	uint32_t deEnergiseKpaScaled;

	/**
 	 * The boost pressure, in Kpa and relative to standard atmosphere, above which the boost controller duty cycle is
	 * controlled by the PID algorithm. Scaled by 1000.
 	 */
	uint32_t pidActiveKpaScaled;

	/** PID proportional constant. Scaled by 1000. */
	uint32_t pidPropConstScaled;

	/** PID integration constant. Scaled by 1000. */
	uint32_t pidIntegConstScaled;

	/** PID derivative constant. Scaled by 1000. */
	uint32_t pidDerivConstScaled;

	/** Maximum duty cycle the solenoid can be set at. Scaled by 10. */
	uint32_t maxDuty;

	/**
	 * Zero point duty cycle of the solenoid. Scaled by 10.
	 * This is essentially the duty cycle that, if set, would result in a steady state of the required boost.
 	 * If this value is too small, it will cause the boost to undershoot the target. If it is too large, it will overshoot.
	 */
	uint32_t zeroPointDuty;
};

#endif