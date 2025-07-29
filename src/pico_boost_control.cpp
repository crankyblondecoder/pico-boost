#include "pico/time.h"

#include "BoschMap_0261230119.hpp"
#include "gpioAlloc.hpp"
#include "pico_boost_control.hpp"
#include "PicoAdcReader.hpp"
#include "PicoPwm.hpp"

/** Standard atmospheric pressure in Pascals. */
#define STD_ATM_PRESSURE 101325

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

/** Frequency of control solenoid. */
#define CONTROL_SOLENOID_FREQ 30

/** Gate state to use when disabling PWM. */
#define CONTROL_SOLENOID_DISABLE_GATE_STATE false

/**
 * De-energise/energise hysteresis to stop rapid flucations with enable/disable of PWM around that point.
 * In KPA, scaled by 1000.
 */
#define DE_ENERGISE_HYSTERESIS 5000

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

/**
 * The pressure, in Kpa and relative to standard atmosphere, above which the boost controller duty cycle is controlled by
 * the PID algorithm. Scaled by 1000.
 */
uint32_t boost_pid_active_kpa_scaled = 75000;

/**
 * The baseline solenoid duty cycle. This is the value that corresponds to a PID error of 0.
 * If this value is too small, it will cause the boost to undershoot the target. If it is too large, it will overshoot.
 * It is probably also dependent on the turbine inlet temperature which changes the characteristics of the waste gate in
 * terms of how much it opening affects the turbine power output.
 */
uint32_t boost_pid_baseline_duty_scaled = 50000;

/** Whether the solenoid is currently energised. ie PWM is active. */
bool boost_energised = false;

/** Whether solenoid is being controlled by the PID algorithm. */
bool boost_pid_active = false;

/** Previous error for the PID algorithm. */
float boost_pid_prev_error = 0;

/** Current integral value for the PID algorithm. */
float boost_pid_integ = 0;

/**
 * PID proportional constant Kp. Scaled by 1000.
 * This initial value should cause an above max duty when the PID algorithm initially kicks in.
 */
uint32_t boost_pid_prop_const_scaled = 6000;

/** PID integration constant Ki. Scaled by 1000. */
uint32_t boost_pid_integ_const_scaled = 1000;

/** PID derivative constant Kd. Scaled by 1000. */
uint32_t boost_pid_deriv_const_scaled = 500;

/** Maximum duty cycle, in %. Scaled by 10. */
uint32_t boost_max_duty = 950;

/**
 * Duty cycle that corresponds to PID error being 0, in %. Scaled by 10.
 * This is essentially the duty cycle that, if set, would result in a steady state of the required boost.
 */
uint32_t boost_zero_point_duty = 200;

/** Current duty cycle. Scaled by 1000. */
uint32_t boost_current_duty_scaled = 0;

/** The next time the boost map sensor is latched (reads and stores the current value of the sensor) */
absolute_time_t next_boost_latch_time;

/** The next time the current boost value is read. This is typically averaged across several latched values. */
absolute_time_t next_boost_read_time;

/** The last time the solenoid duty cycle was processed. */
absolute_time_t last_solenoid_proc_time;

/** PWM control. Assume N Channel Mosfet (IRLZ34N) is being used and the gate must be pulled to ground. */
PicoPwm pwmControl(CONTROL_SOLENOID_CHAN_A_GPIO, CONTROL_SOLENOID_CHAN_A_GPIO + 1, CONTROL_SOLENOID_FREQ, 0, 0, true,
	CONTROL_SOLENOID_DISABLE_GATE_STATE);

/** Current energised state of the boost control solenoid. Does NOT include 0% duty cycle. */
bool energised = false;

/** Whether test mode is currently active. */
bool test_mode = false;

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
	next_boost_read_time = next_boost_latch_time;
	last_solenoid_proc_time = next_boost_latch_time;
}

void boost_control_poll()
{
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= next_boost_latch_time)
	{
		// Latch data at approximately 1000hz. This is a higher frequency to allow for averaging to be effective.
		next_boost_latch_time = delayed_by_ms(next_boost_latch_time, 1);

		boost_map_sensor -> latch();
	}

	if(debug || curTime >= next_boost_read_time)
	{
		// Process map sensor and control solenoid at approximately 100hz
		next_boost_read_time = delayed_by_ms(next_boost_read_time, 10);

		boost_map_kpa_scaled = boost_map_sensor -> readKpa() * 1000.0;

		double boost_atm_scaled = (double)boost_map_kpa_scaled - STD_ATM_PRESSURE;

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
	return boost_control_get_kpa_scaled() >= (int)boost_max_kpa_scaled;
}

int boost_control_get_kpa_scaled()
{
	return boost_map_kpa_scaled - STD_ATM_PRESSURE;
}

int boost_control_get_psi_scaled()
{
	return (boost_control_get_kpa_scaled() / (float)1000.0) * KPA_TO_PSI * 10;
}

unsigned boost_control_get_max_kpa_scaled()
{
	return boost_max_kpa_scaled;
}

void boost_control_set_max_kpa_scaled(unsigned maxKpaScaled)
{
	boost_max_kpa_scaled = maxKpaScaled;
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

void boost_control_set_de_energise_kpa_scaled(unsigned kpaScaled)
{
	boost_de_energise_kpa_scaled = kpaScaled;
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

void boost_control_set_pid_prop_const_scaled(unsigned pidPropConstScaled)
{
	boost_pid_prop_const_scaled = pidPropConstScaled;
}

void boost_control_alter_pid_prop_const_scaled(int pidPropConstScaledDelta)
{
	int newVal = (int)boost_pid_prop_const_scaled + pidPropConstScaledDelta;

	if(newVal > 0) boost_pid_prop_const_scaled = newVal; else boost_pid_prop_const_scaled = 0;
}

unsigned boost_control_get_pid_integ_const_scaled()
{
	return boost_pid_integ_const_scaled;
}

void boost_control_set_pid_integ_const_scaled(unsigned pidIntegConstScaled)
{
	boost_pid_integ_const_scaled = pidIntegConstScaled;
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

void boost_control_set_pid_deriv_const_scaled(unsigned pidDerivConstScaled)
{
	boost_pid_deriv_const_scaled = pidDerivConstScaled;
}

void boost_control_alter_pid_deriv_const_scaled(int pidDerivConstScaledDelta)
{
	int newVal = (int)boost_pid_deriv_const_scaled + pidDerivConstScaledDelta;

	if(newVal > 0) boost_pid_deriv_const_scaled = newVal; else boost_pid_deriv_const_scaled = 0;
}

unsigned boost_control_get_max_duty_scaled()
{
	return boost_max_duty;
}

void boost_control_set_max_duty_scaled(unsigned maxDuty)
{
	boost_max_duty = maxDuty;

	if(boost_max_duty > 1000) boost_max_duty = 1000;
}

void boost_control_alter_max_duty_scaled(int maxDutyDelta)
{
	int newVal = (int)boost_max_duty + maxDutyDelta;

	if(newVal > 0) boost_max_duty = newVal; else boost_max_duty = 0;

	if(boost_max_duty > 1000) boost_max_duty = 1000;
}

unsigned boost_control_get_zero_point_duty_scaled()
{
	return boost_zero_point_duty;
}

void boost_control_set_zero_point_duty_scaled(unsigned zeroPointDuty)
{
	boost_zero_point_duty = zeroPointDuty;

	if(boost_zero_point_duty > 1000) boost_zero_point_duty = 1000;
}

void boost_control_alter_zero_point_duty_scaled(int dutyDelta)
{
	int newVal = (int)boost_zero_point_duty + dutyDelta;

	if(newVal > 0) boost_zero_point_duty = newVal; else boost_zero_point_duty = 0;

	if(boost_zero_point_duty > 1000) boost_zero_point_duty = 1000;
}

unsigned boost_control_get_current_duty_scaled()
{
	return boost_current_duty_scaled;
}

/**
 * Set percentage duty cycle.
 */
void set_solenoid_duty(float duty)
{
	pwmControl.setDuty(duty, -1);
}

void enable_solenoid()
{
	pwmControl.enable();
}

void disable_solenoid()
{
	pwmControl.disable(CONTROL_SOLENOID_DISABLE_GATE_STATE);
}

void process_control_solenoid()
{
	if(!test_mode)
	{
		uint32_t curBoostScaled = boost_map_kpa_scaled - STD_ATM_PRESSURE;

		// Apply hysterisis to enable/disable about de-energise point.
		if(boost_energised && curBoostScaled < (boost_de_energise_kpa_scaled - DE_ENERGISE_HYSTERESIS))
		{
			boost_energised = false;
			disable_solenoid();
		}
		else if(!boost_energised && curBoostScaled > boost_de_energise_kpa_scaled)
		{
			boost_energised = true;
			enable_solenoid();
		}

		if(boost_energised)
		{
			if(curBoostScaled < boost_pid_active_kpa_scaled)
			{
				// Just pin at max duty.
				set_solenoid_duty(boost_max_duty);

				boost_pid_active = false;
			}
			else
			{
				if(!boost_pid_active)
				{
					// TODO ... Setup initial PID vars.
					//blah;

					boost_pid_active = true;
				}

				// TODO ... PID control
				//blah;
			}
		}
		else
		{
			boost_pid_active = false;
		}
	}
}

void boost_control_test_solenoid()
{
	test_mode = true;

	enable_solenoid();

	for(float duty = 0; duty < 100; duty += 1)
	{
		set_solenoid_duty(duty);
		sleep_ms(100);
	}

	/*
	set_solenoid_duty(5);
	sleep_ms(60000);
	set_solenoid_duty(0);
	sleep_ms(1000);
	*/

	disable_solenoid();

	test_mode = false;
}

double boost_map_read_supply_voltage()
{
	return boost_map_sensor -> readSupplyVoltage();
}

double boost_map_read_sensor_voltage()
{
	return boost_map_sensor -> readSensorVoltage();
}