#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"

#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

/** Up and select button. */
#define UP_SELECT_BUTTON_GPIO 14

/** Down button */
#define DOWN_BUTTON_GPIO 15

void boost_options_init()
{
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

	bool lastSelectState = false;

	uint32_t last_kpa = 0;

	// Display probe next render time. Stops thrashing of the display.
	absolute_time_t nextDisplayRenderTime = get_absolute_time();
}

void boost_options_poll()
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