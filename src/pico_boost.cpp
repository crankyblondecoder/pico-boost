#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/regs/intctrl.h"

#include "bosch_map_0261230119.hpp"
#include "TM1637_pico.hpp"

#define ADC_MAP_INPUT_GPIO 26
#define SELECT_BUTTON_GPIO 15

bool debugMsgActive = true;

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

	printf("Pico has initialised.\n");

	bool lastSelectState = false;

	// Current test display number.
	unsigned dispNum = 0;
	// Test display alpha.
	char testAlpha[] = {'a', 'b', 'c', 'd', 'e', 'f', 'h', 'j', 'l', 'o', 'p', 'q', 'r', 'u', 'y'};
	// Current test display alpha index.
	unsigned alphaPosn = 0;

	unsigned int_kpa = 0;

	bosch_map_0261230119 mapSensor(0);

	// Main processing loop.
	while(1)
	{
		unsigned new_int_kpa = mapSensor.readKpa();

		if(new_int_kpa != int_kpa)
		{
			int_kpa = new_int_kpa;

			// Peel off 3 integer digits and display.

			dispData[3] = display.encodeDigit(int_kpa % 10);
			int_kpa /= 10;

			dispData[2] = display.encodeDigit(int_kpa % 10);
			int_kpa /= 10;

			dispData[1] = display.encodeDigit(int_kpa % 10);
			int_kpa /= 10;

			dispData[0] = 0;

			display.show(dispData);
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
