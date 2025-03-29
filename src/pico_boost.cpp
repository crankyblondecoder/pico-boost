#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/time.h"
#include "hardware/regs/intctrl.h"

#include "TM1637_pico.hpp"

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

	// Create 4 digit display instance.
	TM1637Display display(21, 22);

	// Display data.
	uint8_t dispData[4];

	dispData[0] = display.encodeDigit(0);
	dispData[1] = display.encodeDigit(1);
	dispData[2] = display.encodeDigit(2);
	dispData[3] = display.encodeDigit(3);

	// GPIO Select button.
	gpio_init(SELECT_BUTTON_GPIO);
	gpio_set_dir(SELECT_BUTTON_GPIO, GPIO_IN);

	printf("Pico has initialised.\n");

	// Main processing loop.
	while(1)
	{
		// TODO ...
	}
}
