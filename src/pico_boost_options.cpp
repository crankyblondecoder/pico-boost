#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"

#include "pico_boost_options.hpp"

extern bool debug;

extern uint32_t boost_map_kpa_scaled;

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

/** Up and select button. */
#define UP_SELECT_BUTTON_GPIO 14

/** Down button */
#define DOWN_BUTTON_GPIO 15

/** Amount of time of inactivity, in milliseconds, before the current mode ends. */
#define MODE_CHANGE_TIMEOUT = 5000;

/** Amount of time, in milliseconds, that the select button must be pressed to enter edit mode. */
#define MODE_ENTER_EDIT_TIME = 3000;

/** Button for select/up operations. */
PicoSwitch* select_up_button;

/** Button for down operation. */
PicoSwitch* down_button;

/** 4 Digit display. */
TM1637Display* display;

/** Data transfered to display. */
uint8_t disp_data[4];

/** Last displayed boost value. In KPa. */
uint32_t last_disp_kpa = 0;

/** Next render time for default display. Stops flickering of the display when current boost levels fluctuate. */
absolute_time_t nextDefaultDisplayRenderTime;

/** Currently selected option. */
enum SelectOption curSelectedOption = CURRENT_BOOST;

/** Whether edit mode is active. */
bool edit_mode = false;

void _display_default();

void boost_options_init()
{
	// Create 4 digit display instance.
	display = new TM1637Display(16, 17);

	// Initial display data.
	disp_data[3] = display -> encodeDigit(0);
	disp_data[2] = 0;
	disp_data[1] = 0;
	disp_data[0] = 0;

	display -> show(disp_data);

	// Up/Select mode button.
	select_up_button = new PicoSwitch(UP_SELECT_BUTTON_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	// Down button.
	down_button = new PicoSwitch(DOWN_BUTTON_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	nextDefaultDisplayRenderTime = get_absolute_time();
}

void boost_options_poll()
{
	// Note: Make sure the polling frequency is high enough that switches can debounce.
	select_up_button -> poll();
	down_button -> poll();

	// Normal non-options display is active.
	absolute_time_t curTime = get_absolute_time();

	switch(curSelectedOption)
	{
		case CURRENT_BOOST:

			_display_default();
			break;

		case BOOST_MAX_KPA:


		case BOOST_DE_ENERGISE_KPA:

			break;

		case BOOST_PID_PROP_CONST:

			break;

		case BOOST_PID_INTEG_CONST:

			break;

		case BOOST_PID_DERIV_CONST:

			break;

		case BOOST_MAX_DUTY:

			break;
	}
}

void _display_default()
{
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= nextDefaultDisplayRenderTime)
	{
		// Approx 30fps.

		nextDefaultDisplayRenderTime = delayed_by_ms(nextDefaultDisplayRenderTime, 200);

		if(boost_map_kpa_scaled != last_disp_kpa)
		{
			last_disp_kpa = boost_map_kpa_scaled;

			// Show kpa with 0 decimal points.
			unsigned dispKpa = boost_map_kpa_scaled / 1000;

			// Display just 3 digits of kPa value.
			display -> encodeNumber(dispKpa, 3, 3, disp_data);

			display -> show(disp_data);
		}
	}
}
