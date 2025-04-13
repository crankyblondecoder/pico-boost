#include "pico/time.h"

#include "bosch_map_0261230119.hpp"

#include "pico_boost_control.hpp"

/** Bosch map sensor to read current turbo pressure from. */
bosch_map_0261230119* boostMapSensor;

/**
 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
 */
uint32_t boostMapKpaScaled = 0;

/** The maximum boost, in Kpa, scaled by 1000. */
uint32_t boostMaxKpaScaled = 100000;

/** The pressure, in Kpa, below which the boost controller is de-energised. */
uint32_t boostDeEnergiseKpaScaled = 50000;

absolute_time_t nextBoostLatchTime;
absolute_time_t nextBoostReadTime;

extern bool debug;

void boost_control_init()
{
	// Start map sensor on ADC Channel 0. This should map to GP26.
	boostMapSensor = new bosch_map_0261230119(0, 10);

	nextBoostLatchTime = get_absolute_time();
	nextBoostReadTime = get_absolute_time();
}

void boost_control_poll()
{
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= nextBoostLatchTime)
	{
		// Latch data at approximately 1000hz
		nextBoostLatchTime = delayed_by_ms(nextBoostLatchTime, 1);

		boostMapSensor -> latch();
	}

	if(debug || curTime >= nextBoostReadTime)
	{
		// Report data at approximately 100hz
		nextBoostReadTime = delayed_by_ms(nextBoostReadTime, 10);

		boostMapKpaScaled = boostMapSensor -> readKpa() * 1000.0;
	}
}
