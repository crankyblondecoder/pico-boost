#include "pico/time.h"

#include "BoschMap_0261230119.hpp"
#include "PicoAdcReader.hpp"

#include "pico_boost_control.hpp"

/** Standard atmospheric pressure in Pascals. */
#define STD_ATM_PRESSURE 101325

/** Bosch map sensor to read current turbo pressure from. */
BoschMap_0261230119* boost_map_sensor;

PicoAdcReader* vsys_ref_adc;

/**
 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
 * @note This is an absolute pressure reading.
 */
uint32_t boost_map_kpa_scaled = 0;

/** The maximum boost, in Kpa and relative to standard atmosphere. Scaled by 1000. */
uint32_t boost_max_kpa_scaled = 100000;

/**
 * The pressure, in Kpa and relative to standard atmosphere, below which the boost controller is de-energised. Scaled by 1000.
 */
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

/** Current energised state of the boost control solenoid. Does NOT include 0% duty cycle. */
bool energised = false;

extern bool debug;

/** Process the control solenoid parameters and energise it accordingly. */
void process_control_solenoid();

void boost_control_init()
{
	// Create ADC reader to read VSys. The Pico divides this voltage by 3.
	vsys_ref_adc = new PicoAdcReader(3, 10, 3.0, 3.0);

	// Start map sensor on ADC Channel 0. This should map to GP26.
	// Voltage divider scale is calculated from (R1 + R2) / R2.
	boost_map_sensor = new BoschMap_0261230119(0, 3.0, (2.2 + 3.2) / 3.2, vsys_ref_adc);

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
		// Process map sensor and control solenoid at approximately 100hz
		next_boost_read_time = delayed_by_ms(next_boost_read_time, 10);

		boost_map_kpa_scaled = boost_map_sensor -> readKpa() * 1000.0;

		double boost_atm_scaled = boost_map_kpa_scaled - STD_ATM_PRESSURE;

		energised = boost_atm_scaled >= boost_de_energise_kpa_scaled;

		process_control_solenoid();
	}
}

bool boost_control_is_energised()
{
	return energised;
}

bool boost_control_max_boost_reached()
{
	return boost_control_get_kpa_scaled() >= boost_max_kpa_scaled;
}

unsigned boost_control_get_kpa_scaled()
{
	int retVal = boost_map_kpa_scaled - STD_ATM_PRESSURE;

	if(retVal > 0) return retVal;

	return 0;
}

unsigned boost_control_get_max_kpa_scaled()
{
	return boost_max_kpa_scaled;
}

void boost_control_alter_max_kpa_scaled(int maxBoostKpaScaledDelta)
{
	int newVal = (int)boost_max_kpa_scaled + maxBoostKpaScaledDelta;

	if(newVal > 0) boost_max_kpa_scaled = newVal; else boost_max_kpa_scaled = 0;
}

unsigned boost_control_get_de_energise_kpa_scaled()
{
	return boost_de_energise_kpa_scaled;
}

void boost_control_alter_de_energise_kpa_scaled(int deEnergiseKpaScaledDelta)
{
	int newVal = (int)boost_de_energise_kpa_scaled + deEnergiseKpaScaledDelta;

	if(newVal > 0) boost_de_energise_kpa_scaled = newVal; else boost_de_energise_kpa_scaled = 0;
}

unsigned boost_control_get_pid_prop_const_scaled()
{
	return boost_pid_prop_const_scaled;
}

void boost_control_alter_pid_prop_const_scaled(int pidPropConstScaledDelta)
{
	int newVal = (int)boost_pid_prop_const_scaled + pidPropConstScaledDelta;

	if(newVal < 0) boost_pid_prop_const_scaled = 0;
}

unsigned boost_control_get_pid_integ_const_scaled()
{
	return boost_pid_integ_const_scaled;
}

void boost_control_alter_pid_integ_const_scaled(int pidIntegConstScaledDelta)
{
	int newVal = (int)boost_pid_integ_const_scaled + pidIntegConstScaledDelta;

	if(newVal > 0) boost_pid_integ_const_scaled = newVal; else boost_pid_integ_const_scaled = 0;
}

unsigned boost_control_get_pid_deriv_const_scaled()
{
	return boost_pid_deriv_const_scaled;
}

void boost_control_alter_pid_deriv_const_scaled(int pidDerivConstScaledDelta)
{
	int newVal = (int)boost_pid_deriv_const_scaled + pidDerivConstScaledDelta;

	if(newVal > 0) boost_pid_deriv_const_scaled = newVal; else boost_pid_deriv_const_scaled = 0;
}

unsigned boost_control_get_max_duty()
{
	return boost_max_duty;
}

void boost_control_alter_max_duty(int maxDutyDelta)
{
	int newVal = (int)boost_max_duty + maxDutyDelta;

	if(newVal > 0) boost_max_duty = newVal; else boost_max_duty = 0;
}

void process_control_solenoid()
{
	// TODO ...
}