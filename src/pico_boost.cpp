#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/regs/intctrl.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "pico_boost_control.hpp"
#include "pico_boost_options.hpp"

/** The ADC channel used to get VSYS voltage. */
#define VSYS_REF_CHANNEL 3

bool debugMsgActive = true;

// Debug flag to get around timers being a pain. Set during debug only.
bool debug = false;

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

	// Initialise the options module. Do this first so that boost control variables are set.
	boost_options_init();

	// Start second core which will read sensor data and control the wastegate solenoid.
	multicore_launch_core1(__core1_entry);

	// Main processing loop. Used for user interaction.
	while(1)
	{
		boost_options_poll();
	}
}

void __core1_entry()
{
	boost_control_init();

	while(1)
	{
		boost_control_poll();
	}
}
