#include "pico/time.h"

#include "BoschMap_0261230119.hpp"
#include "PicoAdcReader.hpp"

#include "pico_boost_control.hpp"

/** Bosch map sensor to read current turbo pressure from. */
BoschMap_0261230119* boost_map_sensor;

PicoAdcReader* vsys_ref_adc;

/**
 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
 */
uint32_t boost_map_kpa_scaled = 0;

/** The maximum boost, in Kpa. Scaled by 1000. */
uint32_t boost_max_kpa_scaled = 100000;

/** The pressure, in Kpa, below which the boost controller is de-energised. Scaled by 1000. */
uint32_t boost_de_energise_kpa_scaled = 50000;

/** PID proportional constant Kp. Scaled by 1000. */
uint32_t boost_pid_prop_const_scaled = 500;

/** PID integration constant Ki. Scaled by 1000. */
uint32_t boost_pid_integ_const_scaled = 500;

/** PID derivative constant Kd. Scaled by 1000. */
uint32_t boost_pid_deriv_const_scaled = 500;

/** Maximum duty cycle, in %. */
uint32_t boost_max_duty = 100;

absolute_time_t next_boost_latch_time;
absolute_time_t next_boost_read_time;

extern bool debug;

void boost_control_init()
{
	// Create ADC reader to read VSys. The Pico divides this voltage by 3.
	vsys_ref_adc = new PicoAdcReader(4, 10, 3.0, 3.0);

	// Start map sensor on ADC Channel 0. This should map to GP26.
	boost_map_sensor = new BoschMap_0261230119(0, vsys_ref_adc);

	next_boost_latch_time = get_absolute_time();
	next_boost_read_time = get_absolute_time();
}

void boost_control_poll()
{
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= next_boost_latch_time)
	{
		// Latch data at approximately 1000hz
		next_boost_latch_time = delayed_by_ms(next_boost_latch_time, 1);

		boost_map_sensor -> latch();
	}

	if(debug || curTime >= next_boost_read_time)
	{
		// Report data at approximately 100hz
		next_boost_read_time = delayed_by_ms(next_boost_read_time, 10);

		boost_map_kpa_scaled = boost_map_sensor -> readKpa() * 1000.0;
	}
}
