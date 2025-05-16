#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "Eeprom_24CS256.hpp"

#include "pico_boost_control.hpp"
#include "pico_boost_options.hpp"

extern bool debug;

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

/** Up and select button. */
#define UP_SELECT_BUTTON_GPIO 14

/** Down button */
#define DOWN_BUTTON_GPIO 15

/** Amount of time of inactivity, in milliseconds, before the current mode ends and the system returns to default. */
#define MODE_COMPLETE_TIMEOUT 5000

/** Amount of time, in milliseconds, that the select button must be pressed to enter edit mode. */
#define MODE_ENTER_EDIT_TIME 3000

/** Amount of time, in milliseconds, that either up or down button must be pressed for to go into "fast" mode. */
#define EDIT_MODE_FAST_TIME 1500

/** Time in milliseconds between edit fast mode data changes. */
#define EDIT_MODE_FAST_REPEAT_RATE 100

/** Time in milliseconds of display "flashing" toggle. */
#define DISPLAY_FLASH_PERIOD 500

/** Time between display refresh, in milliseconds. */
#define DISPLAY_FRAME_RATE 50

/** Button for select/up operations. */
PicoSwitch* select_up_button;

/** The last _fully_ procesed select/up button state index. */
unsigned last_proc_select_up_button_state_index = 0;

/** Button for down operation. */
PicoSwitch* down_button;

/** The last _fully_ processed down button state index. */
unsigned last_proc_down_button_state_index = 0;

/** 4 Digit display. */
TM1637Display* display;

/** Data transfered to display. */
uint8_t disp_data[4];

/** Next render time for display. Stops flickering of the display when values change rapidly. */
absolute_time_t nextDisplayRenderTime;

/** Currently selected option. */
int cur_selected_option = CURRENT_BOOST;

/** Whether edit mode is active. */
bool edit_mode = false;

/** During edit mode fast mode, the last up switch duration that was acted upon. */
unsigned last_edit_fast_mode_up_duration = 0;

/** During edit mode fast mode, the last down switch duration that was acted upon. */
unsigned last_edit_fast_mode_down_duration = 0;

/** Flag to indicate whether display is on in the display "flashing" cycle. */
bool displayFlashOn = true;

/** Next absolute time to toggle the current display flash flag. */
absolute_time_t nextDisplayFlashToggleTime = 0;

/** 24CS256 EEPROM responding to address 0 on i2c bus 0. */
Eeprom_24CS256* eeprom_24CS256;

void __testEeprom();

void display_current_boost();
void display_max_boost();
void display_boost_de_energise();
void display_boost_pid_prop_const();
void display_boost_pid_integ_const();
void display_boost_pid_deriv_const();
void display_boost_max_duty();

// Display character mapping
// -------------------------
// E: Default display energised.
// L: Default display max boost reached.
// B: BOOST_MAX_KPA
// U: BOOST_DE_ENERGISE_KPA
// P: BOOST_PID_PROP_CONST
// J: BOOST_PID_INTEG_CONST
// D: BOOST_PID_DERIV_CONST
// Q: BOOST_MAX_DUTY

void boost_options_process_switches()
{
	unsigned curSelectUpStateIndex = select_up_button -> getCurrentStateCycleIndex();
	unsigned curDownStateIndex = down_button -> getCurrentStateCycleIndex();

	bool selectUpStateUnProc = curSelectUpStateIndex != last_proc_select_up_button_state_index;
	bool downStateUnProc = curDownStateIndex != last_proc_down_button_state_index;

	// Full processed flags.
	bool selectUpProced = false;
	bool downProced = false;

	// Regardless of the current selected option, both switches not being pressed for greater than the mode complete timeout
	// puts the system back into a default mode.
	if(!select_up_button -> getSwitchState() && !down_button -> getSwitchState() &&
		select_up_button -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		down_button -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT)
	{
		cur_selected_option = CURRENT_BOOST;
		edit_mode = false;

		selectUpProced = true;
		downProced = true;
	}
	else if(!edit_mode)
	{
		if(selectUpStateUnProc)
		{
			// Look for edit mode entry, which can't happen in (default) boost display mode.
			if(cur_selected_option != CURRENT_BOOST && select_up_button -> getSwitchState() &&
				select_up_button -> getSwitchStateDuration() > MODE_ENTER_EDIT_TIME && !down_button -> getSwitchState())
			{
				edit_mode = true;

				selectUpProced = true;
				downProced = true;
			}
			else if(!select_up_button -> getSwitchState())
			{
				// Change to the next state on select button release.
				cur_selected_option++;

				if(cur_selected_option >= SELECT_OPTION_LAST) cur_selected_option = CURRENT_BOOST;

				selectUpProced = true;
				downProced = true;
			}
		}
	}
	else
	{
		// Should be in edit mode.

		// Both buttons pressed at the same time exit edit mode.
		if(select_up_button -> getSwitchState() && down_button -> getSwitchState())
		{
			edit_mode = false;

			selectUpProced = true;
			downProced = true;

			// TODO ... Trigger options save to EEPROM.
		}
		else
		{
			// Note: Non fast edit mode relies on button release to trigger change otherwise fast edit mode won't be detected
			// and edit mode exit will cause a simultaneous change in values which isn't desired.

			int delta = 0;

			if(selectUpStateUnProc)
			{
				bool switchState = select_up_button -> getSwitchState();

				unsigned upDuration = select_up_button -> getSwitchStateDuration();

				if(switchState && upDuration > EDIT_MODE_FAST_TIME)
				{
					// Rate limit fast edit mode.
					if(upDuration - last_edit_fast_mode_up_duration > EDIT_MODE_FAST_REPEAT_RATE)
					{
						delta = 1;
						last_edit_fast_mode_up_duration = upDuration;
					}

					// Note: Button is left as unprocessed so this block can be re-entered.
				}
				else if(!switchState)
				{
					delta = 1;

					// Button is full processed.
					selectUpProced = true;
				}
				else
				{
					// This should invoke fast edit change immediately.
					last_edit_fast_mode_up_duration = 0;
				}
			}

			if(downStateUnProc)
			{
				bool switchState = down_button -> getSwitchState();

				unsigned downDuration = down_button -> getSwitchStateDuration();

				if(switchState && downDuration > EDIT_MODE_FAST_TIME)
				{
					// Rate limit fast edit mode.
					if(downDuration - last_edit_fast_mode_down_duration > EDIT_MODE_FAST_REPEAT_RATE)
					{
						delta = -1;
						last_edit_fast_mode_down_duration = downDuration;
					}

					// Note: Button is left as unprocessed so this block can be re-entered.
				}
				else if(!switchState)
				{
					delta = -1;

					downProced = true;
				}
				else
				{
					// This should invoke fast edit change immediately.
					last_edit_fast_mode_down_duration = 0;
				}
			}

			if(delta != 0)
			{
				switch(cur_selected_option)
				{
					case BOOST_MAX_KPA:

						// Max kPa is scaled by 1000. Resolution 1.
						boost_control_alter_max_kpa_scaled(delta * 1000);
						break;

					case BOOST_DE_ENERGISE_KPA:

						// De-energise is scaled by 1000. Resolution 1.
						boost_control_alter_de_energise_kpa_scaled(delta * 1000);
						break;

					case BOOST_PID_PROP_CONST:

						// PID proportional constant is scaled by 1000. Resolution 0.01.
						boost_control_alter_pid_prop_const_scaled(delta * 10);
						break;

					case BOOST_PID_INTEG_CONST:

						// PID integration constant is scaled by 1000. Resolution 0.01.
						boost_control_alter_pid_integ_const_scaled(delta * 10);
						break;

					case BOOST_PID_DERIV_CONST:

						// PID derivative constant is scaled by 1000. Resolution 0.01.
						boost_control_alter_pid_deriv_const_scaled(delta * 10);
						break;

					case BOOST_MAX_DUTY:

						// Boost solenoid maximum duty cycle. Resolution 1.
						boost_control_alter_max_duty(delta);
						break;
				}
			}
		}
	}

	// Save the indexes of any fully processed buttons.
	if(selectUpProced) last_proc_select_up_button_state_index = curSelectUpStateIndex;
	if(downProced) last_proc_down_button_state_index = curDownStateIndex;
}

void boost_options_init()
{
	// Create EEPROM instance and initialise it.
	// TODO Currently don't setup pages so basic read/write tests can be peformed.
	eeprom_24CS256 = new Eeprom_24CS256(i2c0, 0, 0, 0);

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

	nextDisplayRenderTime = get_absolute_time();

	nextDisplayFlashToggleTime = nextDisplayRenderTime;

	// TODO TEMP Test EEPROM
	__testEeprom();
}

void boost_options_poll()
{
	// Note: Make sure the polling frequency is high enough that switches can debounce.
	select_up_button -> poll();
	down_button -> poll();

	boost_options_process_switches();

	// Normal non-options display is active.
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= nextDisplayRenderTime)
	{
		// Process current display flashing toggle.
		if(curTime > nextDisplayFlashToggleTime) {

			displayFlashOn = !displayFlashOn;

			nextDisplayFlashToggleTime = delayed_by_ms(nextDisplayFlashToggleTime, DISPLAY_FLASH_PERIOD);
		}

		// Limit the frame rate so the display doesn't "strobe".
		nextDisplayRenderTime = delayed_by_ms(nextDisplayRenderTime, DISPLAY_FRAME_RATE);

		switch(cur_selected_option)
		{
			case CURRENT_BOOST:

				display_current_boost();
				break;

			case BOOST_MAX_KPA:

				display_max_boost();
				break;

			case BOOST_DE_ENERGISE_KPA:

				display_boost_de_energise();
				break;

			case BOOST_PID_PROP_CONST:

				display_boost_pid_prop_const();
				break;

			case BOOST_PID_INTEG_CONST:

				display_boost_pid_integ_const();
				break;

			case BOOST_PID_DERIV_CONST:

				display_boost_pid_deriv_const();
				break;

			case BOOST_MAX_DUTY:

				display_boost_max_duty();
				break;
		}
	}
}

void display_current_boost()
{
	int boost_kpa_scaled = boost_control_get_kpa_scaled();

	// Show kpa with 0 decimal points.
	int dispKpa = boost_kpa_scaled / 1000;
	if(dispKpa < 0) dispKpa *= -1;

	// Display just 3 digits of kPa value.
	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(boost_control_max_boost_reached())
	{
		disp_data[0] = display -> encodeAlpha('L');
	}
	else if(boost_control_is_energised())
	{
		disp_data[0] = display -> encodeAlpha('E');
	}
	else
	{
		if(boost_kpa_scaled <= -1000)
		{
			disp_data[0] = display -> encodeAlpha('-');
		}
		else
		{
			disp_data[0] = 0;
		}
	}

	display -> show(disp_data);
}

void display_max_boost()
{
	unsigned boost_max_kpa_scaled = boost_control_get_max_kpa_scaled();

	// Show max kpa with 0 decimal points.
	unsigned dispKpa = boost_max_kpa_scaled / 1000;

	// Display just 3 digits of kPa value.
	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('B');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_boost_de_energise()
{
	unsigned boost_de_energised_scaled = boost_control_get_de_energise_kpa_scaled();

	// Show with 0 decimal points.
	unsigned dispKpa = boost_de_energised_scaled / 1000;

	// Display just 3 digits of kPa value.
	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('U');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_boost_pid_prop_const()
{
	unsigned boost_pid_prop_scaled = boost_control_get_pid_prop_const_scaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boost_pid_prop_scaled / 10;

	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('P');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_boost_pid_integ_const()
{
	unsigned boost_pid_integ_scaled = boost_control_get_pid_integ_const_scaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boost_pid_integ_scaled / 10;

	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('J');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_boost_pid_deriv_const()
{
	unsigned boost_pid_deriv_scaled = boost_control_get_pid_deriv_const_scaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boost_pid_deriv_scaled / 10;

	display -> encodeNumber(dispKpa, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('D');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_boost_max_duty()
{
	unsigned boost_max_duty = boost_control_get_max_duty();

	// Show with 0 decimal points.
	unsigned dispVal = boost_max_duty;

	display -> encodeNumber(dispVal, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('Q');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void __testEeprom()
{
	// Read more than 64 bytes (so it crosses the EEPROM page size).

	uint8_t readBuffer[100];

	for(int index = 0; index < 100; index++)
	{
		readBuffer[index] = 0x55;
	}

	eeprom_24CS256 -> readBytes(32, readBuffer, 100);

	bool allGood = true;

	for(int index = 0; index < 100; index++)
	{
		// Should read all 0xFF.
		if(readBuffer[index] != 0xFF)
		{
			allGood = false;
			break;
		}
	}

	if(!allGood)
	{
		printf("EEPROM read test failed.");
	}
	else
	{
		printf("EEPROM read test passed.");
	}
}