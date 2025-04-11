#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/regs/intctrl.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "bosch_map_0261230119.hpp"
#include "pico_boost_options.hpp"
#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

#define ADC_MAP_INPUT_GPIO 26

/** Up and select button. */
#define UP_SELECT_BUTTON_GPIO 14

/** Down button */
#define DOWN_BUTTON_GPIO 15

bool debugMsgActive = true;

// Debug flag to get around timers being a pain.
bool debug = false;

/**
 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
 */
uint32_t boostMapKpaScaled = 0;

/** The maximum boost, in Kpa, scaled by 1000. */
uint32_t boostMaxKpaScaled = 100;

/** The pressure, in Kpa, below which the boost controller is de-energised. */
uint32_t boostDeEnergiseKpaScaled = 50;



void __core1_entry();

/**
 * Program for Pi Pico that interfaces to cars electrical signals and can either provide data to another display system
 * or drive physical gauges directly.
 */
int main()
{
	// Make sure disabled interrupts wake up WFE via the SEVONPEND flag of the SCR register.
	// See "M0PLUS: SCR Register" in RP2040 Datasheet.
	// SEVONPEND needs to be set. This allows WFE to wake up on pending interrupt even if disabled.
	uint32_t* scrRegAddr = (uint32_t*)(PPB_BASE + 0xed10);
	*scrRegAddr |= 0x10;

	// Must happen for serial stdout to work.
	stdio_init_all();

	// Setup ADC.
	adc_init();

	// Push GPIO pin 23 (SMPS Mode) high to disable power saving, which causes noise in the ADC.
	gpio_init(23);
	gpio_set_dir(23, GPIO_OUT);
	gpio_put(23, true);

	// Enable on board LED (non-W)
	gpio_init(25);
	gpio_set_dir(25, GPIO_OUT);
	gpio_put(25, false);

	// Start second core which will read sensor data and control the wastegate solenoid.
	multicore_launch_core1(__core1_entry);

	// Create 4 digit display instance.
	TM1637Display display(16, 17);

	// Display data.
	uint8_t dispData[4];

	// Initial display data.
	dispData[3] = display.encodeDigit(0);
	dispData[2] = 0;
	dispData[1] = 0;
	dispData[0] = 0;

	display.show(dispData);

	// Up/Select mode button.
	PicoSwitch selectButton(UP_SELECT_BUTTON_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	// Down button.
	PicoSwitch downButton(DOWN_BUTTON_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	// Initialise the options.
	init_options();

	bool lastSelectState = false;

	uint32_t last_kpa = 0;

	// Display probe next render time. Stops thrashing of the display.
	absolute_time_t nextDisplayRenderTime = get_absolute_time();

	// Main processing loop. Used for user interaction.
	while(1)
	{
		selectButton.poll();
		downButton.poll();

		if(!process_options(selectButton, downButton, display))
		{
			// Normal non-options display is active.
			absolute_time_t curTime = get_absolute_time();

			if(debug || curTime >= nextDisplayRenderTime)
			{
				// Approx 30fps.

				nextDisplayRenderTime = delayed_by_ms(nextDisplayRenderTime, 200);

				if(boostMapKpaScaled != last_kpa)
				{
					last_kpa = boostMapKpaScaled;

					unsigned dispKpa = boostMapKpaScaled / 100;

					// Peel off 3 integer digits and display.

					dispData[3] = display.encodeDigit(dispKpa % 10);
					dispKpa /= 10;

					dispData[2] = display.encodeDigit(dispKpa % 10);
					dispKpa /= 10;

					dispData[1] = display.encodeDigit(dispKpa % 10);
					dispKpa /= 10;

					dispData[0] = display.encodeDigit(dispKpa % 10);

					display.show(dispData);
				}
			}
		}
	}
}

void __core1_entry()
{
	// Start map sensor on ADC Channel 0. This should map to GP26.
	bosch_map_0261230119 boostMapSensor(0, 10);

	absolute_time_t nextBoostLatchTime = get_absolute_time();
	absolute_time_t nextBoostReadTime = get_absolute_time();

	while(1)
	{
		absolute_time_t curTime = get_absolute_time();

		if(debug || curTime >= nextBoostLatchTime)
		{
			// Latch data at approximately 1000hz
			nextBoostLatchTime = delayed_by_ms(nextBoostLatchTime, 1);

			boostMapSensor.latch();
		}

		if(debug || curTime >= nextBoostReadTime)
		{
			// Report data at approximately 100hz
			nextBoostReadTime = delayed_by_ms(nextBoostReadTime, 10);

			boostMapKpaScaled = boostMapSensor.readKpa() * 1000.0;
		}
	}
}
