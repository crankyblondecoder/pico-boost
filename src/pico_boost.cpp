#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/regs/intctrl.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "bosch_map_0261230119.hpp"
#include "TM1637_pico.hpp"

#define ADC_MAP_INPUT_GPIO 26
#define SELECT_BUTTON_GPIO 15

bool debugMsgActive = true;

/**
 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
 */
uint32_t boostMapKpaScaled = 0;

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

	// GPIO Select mode button.
	gpio_init(SELECT_BUTTON_GPIO);
	gpio_set_dir(SELECT_BUTTON_GPIO, GPIO_IN);
	// Switch must connect to voltage high on being pressed for select to be triggered.
	gpio_pull_down(SELECT_BUTTON_GPIO);

	bool lastSelectState = false;

	uint32_t last_kpa = 0;

	// Display probe next render time. Stops thrashing of the display.
	absolute_time_t nextDisplayRenderTime = get_absolute_time();

	// Main processing loop. Used for user interaction.
	while(1)
	{
		absolute_time_t curTime = get_absolute_time();

		if(curTime >= nextDisplayRenderTime)
		{
			// Approx 30fps.
			//nextDisplayRenderTime = delayed_by_ms(nextDisplayRenderTime, 33);-

			nextDisplayRenderTime = delayed_by_ms(nextDisplayRenderTime, 200);

			if(boostMapKpaScaled != last_kpa)
			{
				last_kpa = boostMapKpaScaled;

				unsigned dispKpa = boostMapKpaScaled / 1000;

				// Peel off 3 integer digits and display.

				dispData[3] = display.encodeDigit(last_kpa % 10);
				last_kpa /= 10;

				dispData[2] = display.encodeDigit(last_kpa % 10);
				last_kpa /= 10;

				dispData[1] = display.encodeDigit(last_kpa % 10);
				last_kpa /= 10;

				dispData[0] = 0;

				display.show(dispData);
			}
		}

		bool selectButtonState = gpio_get(SELECT_BUTTON_GPIO);

		if(lastSelectState != selectButtonState)
		{
			lastSelectState = selectButtonState;

			if(selectButtonState)
			{

			}
		}
	}
}

void __core1_entry()
{
	// Start map sensor on ADC Channel 0. This should map to GP26.
	bosch_map_0261230119 boostMapSensor(0);

	absolute_time_t nextBoostReadTime = get_absolute_time();

	while(1)
	{
		absolute_time_t curTime = get_absolute_time();

		if(curTime >= nextBoostReadTime)
		{
			// Approximately 100hz
			nextBoostReadTime = delayed_by_ms(nextBoostReadTime, 10);

			boostMapKpaScaled = boostMapSensor.readKpa() * 1000.0;
		}
	}
}
