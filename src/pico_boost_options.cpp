#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "pico/time.h"

#include "Eeprom_24CS256.hpp"

#include "pico_boost_control.hpp"
#include "pico_boost_options.hpp"

extern bool debug;

/** Amount of time that both buttons have to be pressed to invoke a test pass. */
#define TEST_START_TIMEOUT 5000

/** Up and select button. */
#define UP_SELECT_BUTTON_GPIO 14

/** Down button */
#define DOWN_BUTTON_GPIO 15

/** Min brightness */
#define MIN_BRIGHTNESS_GPIO 22

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

/** Size of EEPROM page, in bytes, that stores options. */
#define OPTIONS_EEPROM_PAGE_SIZE 48

/** Button for select/up operations. */
PicoSwitch* select_up_button;

/** The last _fully_ procesed select/up button state index. */
unsigned last_proc_select_up_button_state_index = 0;

/** Button for down operation. */
PicoSwitch* down_button;

/** The last _fully_ processed down button state index. */
unsigned last_proc_down_button_state_index = 0;

/** Detect gpio being asserted for minimum display brightness as a switch. */
PicoSwitch* min_brightness_input;

/** 4 Digit display. */
TM1637Display* display;

/** Displays maximum brightness. 0-7. */
uint8_t display_max_brightness = 7;

/** Displays minimum brightness. 0-7. */
uint8_t display_min_brightness = 4;

/** Whether to use minimum brightness for display. */
bool display_use_min_brightness = false;

/** Data transfered to display. */
uint8_t disp_data[4];

/** Next render time for display. Stops flickering of the display when values change rapidly. */
absolute_time_t nextDisplayRenderTime;

/** The default selected option. */
int default_select_option = CURRENT_BOOST_PSI;

/** Currently selected option. */
int cur_selected_option = default_select_option;

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

EepromPage eepromPages[1] = {{OPTIONS_EEPROM_PAGE_SIZE, 64}};

void __runTests();

void display_current_boost_kpa();
void display_current_boost_psi();
void display_current_duty();
void display_max_boost();
void display_boost_de_energise();
void display_boost_pid_prop_const();
void display_boost_pid_integ_const();
void display_boost_pid_deriv_const();
void display_boost_max_duty();
void display_boost_zero_point_duty();
void display_show_max_brightness();
void display_show_min_brightness();

/**
 * Commit the current boost options to EEPROM.
 * @returns If commit was verified as being successful.
 */
bool boost_options_commit();

/**
 * Read the current boost options from EEPROM and set on other modules as appropriate.
 * @returns True if read was successful.
 */
bool boost_options_read();

// Display character mapping
// -------------------------
// E: Default display energised.
// L: Default display max boost reached.
// C: Current solenoid control valve duty cycle.
// B: BOOST_MAX_KPA
// U: BOOST_DE_ENERGISE_KPA
// P: BOOST_PID_PROP_CONST
// J: BOOST_PID_INTEG_CONST
// D: BOOST_PID_DERIV_CONST
// Q: BOOST_MAX_DUTY
// R: DISPLAY_MAX_BRIGHTNESS
// H: DISPLAY_MIN_BRIGHTNESS
// O: BOOST_ZERO_POINT_DUTY

void boost_options_process_switches()
{
	unsigned curSelectUpStateIndex = select_up_button -> getCurrentStateCycleIndex();
	unsigned curDownStateIndex = down_button -> getCurrentStateCycleIndex();

	bool selectUpStateUnProc = curSelectUpStateIndex != last_proc_select_up_button_state_index;
	bool downStateUnProc = curDownStateIndex != last_proc_down_button_state_index;

	// Full processed flags.
	bool selectUpProced = false;
	bool downProced = false;

	// Check for test invokation. Both buttons pressed.
	if(selectUpStateUnProc && downStateUnProc && select_up_button -> getSwitchState() &&
		select_up_button -> getSwitchStateDuration() > TEST_START_TIMEOUT && down_button -> getSwitchState() &&
		down_button -> getSwitchStateDuration() > TEST_START_TIMEOUT)
	{
		__runTests();

		last_proc_select_up_button_state_index = curSelectUpStateIndex;
		last_proc_down_button_state_index = curDownStateIndex;

		return;
	}

	// Regardless of the current selected option, both switches not being pressed for greater than the mode complete timeout
	// puts the system back into a default mode. However solenoid duty cycle being displayed stays as is.
	if(!select_up_button -> getSwitchState() && !down_button -> getSwitchState() &&
		select_up_button -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		down_button -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT)
	{
		if(cur_selected_option != CURRENT_DUTY && cur_selected_option != CURRENT_BOOST_PSI &&
			cur_selected_option != CURRENT_BOOST_KPA)
		{
			cur_selected_option = default_select_option;
		}

		edit_mode = false;

		selectUpProced = true;
		downProced = true;
	}
	else if(!edit_mode)
	{
		if(selectUpStateUnProc || downStateUnProc)
		{
			// Look for edit mode entry, which can't happen in (default) boost display mode or solenoid valve duty display mode.
			if(cur_selected_option > CURRENT_DUTY && select_up_button -> getSwitchState() &&
				select_up_button -> getSwitchStateDuration() > MODE_ENTER_EDIT_TIME && !down_button -> getSwitchState())
			{
				edit_mode = true;

				selectUpProced = true;
				downProced = true;
			}
			else if(selectUpStateUnProc && !select_up_button -> getSwitchState())
			{
				// Change to the next state on select button release.
				cur_selected_option++;

				if(cur_selected_option >= SELECT_OPTION_LAST) cur_selected_option = CURRENT_BOOST_PSI;

				selectUpProced = true;
				downProced = true;
			}
			else if(downStateUnProc && !down_button -> getSwitchState())
			{
				// Change to the previou state on down button release.
				cur_selected_option--;

				if(cur_selected_option < 0) cur_selected_option = SELECT_OPTION_LAST - 1;

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

			// Save options to EEPROM.
			boost_options_commit();
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

						// Boost solenoid maximum duty cycle is scaled by 10. Resolution 0.1.
						boost_control_alter_max_duty_scaled(delta);
						break;

					case BOOST_ZERO_POINT_DUTY:

						// Boost solenoid zero point duty cycle is scaled by 10. Resolution 0.1.
						boost_control_alter_zero_point_duty_scaled(delta);
						break;

					case DISPLAY_MAX_BRIGHTNESS:

						// Maximum brightness 0-7.
						display_max_brightness += delta;
						if(display_max_brightness > 7) display_max_brightness = 7;
						break;

					case DISPLAY_MIN_BRIGHTNESS:

						// Minimum brightness 0-7.
						display_min_brightness += delta;
						if(display_min_brightness > 7) display_min_brightness = 7;
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
	// Current Use wear levelled page of size 32.
	// Current saved boost options size: 24

	eeprom_24CS256 = new Eeprom_24CS256(i2c0, 0, eepromPages, 1);

	// Read initial options.
	boost_options_read();

	// Create 4 digit display instance.
	display = new TM1637Display(16, 17);

	// Initial display data.
	disp_data[3] = display -> encodeDigit(0);
	disp_data[2] = 0;
	disp_data[1] = 0;
	disp_data[0] = 0;

	display -> show(disp_data);

	display -> setBrightness(display_max_brightness);

	// Up/Select mode button.
	select_up_button = new PicoSwitch(UP_SELECT_BUTTON_GPIO, PicoSwitch::PULL_UP, 5, 100);

	// Down button.
	down_button = new PicoSwitch(DOWN_BUTTON_GPIO, PicoSwitch::PULL_UP, 5, 100);

	// Min brightness.
	min_brightness_input = new PicoSwitch(MIN_BRIGHTNESS_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	nextDisplayRenderTime = get_absolute_time();

	nextDisplayFlashToggleTime = nextDisplayRenderTime;
}

void boost_options_poll()
{
	// Note: Make sure the polling frequency is high enough that switches can debounce.
	select_up_button -> poll();
	down_button -> poll();
	min_brightness_input -> poll();

	display_use_min_brightness = min_brightness_input -> getSwitchState();

	boost_options_process_switches();

	// Normal non-options display is active.
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= nextDisplayRenderTime)
	{
		// Set brightness of display.
		if(display_use_min_brightness)
		{
			display -> setBrightness(display_min_brightness);
		}
		else
		{
			display -> setBrightness(display_max_brightness);
		}

		// Process current display flashing toggle.
		if(curTime > nextDisplayFlashToggleTime) {

			displayFlashOn = !displayFlashOn;

			nextDisplayFlashToggleTime = delayed_by_ms(nextDisplayFlashToggleTime, DISPLAY_FLASH_PERIOD);
		}

		// Limit the frame rate so the display doesn't "strobe".
		nextDisplayRenderTime = delayed_by_ms(nextDisplayRenderTime, DISPLAY_FRAME_RATE);

		switch(cur_selected_option)
		{
			case CURRENT_BOOST_KPA:

				display_current_boost_kpa();
				break;

			case CURRENT_BOOST_PSI:

				display_current_boost_psi();
				break;

			case CURRENT_DUTY:

				display_current_duty();
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

			case BOOST_ZERO_POINT_DUTY:

				display_boost_zero_point_duty();
				break;

			case DISPLAY_MAX_BRIGHTNESS:

				display_show_max_brightness();
				break;

			case DISPLAY_MIN_BRIGHTNESS:

				display_show_min_brightness();
				break;
		}
	}
}

void display_current_boost_kpa()
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
		// Accounts for flutter around zero. ie Stops negative sign from flashing randomly.
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

void display_current_boost_psi()
{
	int boost_psi_scaled = boost_control_get_psi_scaled();

	// Show psi with 0 decimal points.
	int dispPsi = boost_psi_scaled / 10;
	if(dispPsi < 0) dispPsi *= -1;

	// Display just 2 digits of psi value.
	display -> encodeNumber(dispPsi, 2, 3, disp_data);

	// Default to nothing in the left most char for now.
	disp_data[0] = 0;

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
		// Accounts for flutter around zero. ie Stops negative sign from flashing randomly.
		if(boost_psi_scaled <= -10)
		{
			disp_data[1] = display -> encodeAlpha('-');
		}
		else
		{
			disp_data[1] = 0;
		}
	}

	display -> show(disp_data);
}

void display_current_duty()
{
	unsigned curDuty = boost_control_get_current_duty_scaled();

	// Show max kpa with 0 decimal points.
	unsigned dispDuty = curDuty / 1000;

	// Display just 3 digits of kPa value.
	display -> encodeNumber(dispDuty, 3, 3, disp_data);

	disp_data[0] = display -> encodeAlpha('C');

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
	unsigned boost_max_duty = boost_control_get_max_duty_scaled();

	// Show with 1 decimal point.
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

void display_boost_zero_point_duty()
{
	unsigned boost_zero_point_duty = boost_control_get_zero_point_duty_scaled();

	// Show with 1 decimal point.
	unsigned dispVal = boost_zero_point_duty;

	display -> encodeNumber(dispVal, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('O');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_show_max_brightness()
{
	// Show with 0 decimal points.
	unsigned dispVal = display_max_brightness;

	display -> encodeNumber(dispVal, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('R');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

void display_show_min_brightness()
{
	// Show with 0 decimal points.
	unsigned dispVal = display_min_brightness;

	display -> encodeNumber(dispVal, 3, 3, disp_data);

	if(!edit_mode || displayFlashOn)
	{
		disp_data[0] = display -> encodeAlpha('H');
	}
	else
	{
		disp_data[0] = 0;
	}

	display -> show(disp_data);
}

/*
	Current Page entries:

		uint32_t boost_max_kpa_scaled
		uint32_t boost_de_energise_kpa_scaled
		uint32_t boost_pid_prop_const_scaled
		uint32_t boost_pid_integ_const_scaled
		uint32_t boost_pid_deriv_const_scaled
		uint32_t boost_max_duty
		uint32_t boost_zero_point_duty
		uint8_t display_max_brightness
		uint8_t display_max_brightness
*/

bool boost_options_commit()
{
	uint8_t readBuffer[OPTIONS_EEPROM_PAGE_SIZE];
	uint8_t writeBuffer[OPTIONS_EEPROM_PAGE_SIZE];

	// Zero the page so that there is not random data written. This is necessary for the checksum to be accurate.
	for(int index = 0; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
	{
		readBuffer[index] = 0;
		writeBuffer[index] = 0;
	}

	// Write 32 bit data to page buffer.

	uint32_t* writeBuffer32 = (uint32_t*) writeBuffer;

	// Remember that the first entry in the buffer is the checksum.

	writeBuffer32[1] = boost_control_get_max_kpa_scaled();
	writeBuffer32[2] = boost_control_get_de_energise_kpa_scaled();
	writeBuffer32[3] = boost_control_get_pid_prop_const_scaled();
	writeBuffer32[4] = boost_control_get_pid_integ_const_scaled();
	writeBuffer32[5] = boost_control_get_pid_deriv_const_scaled();
	writeBuffer32[6] = boost_control_get_max_duty_scaled();
	writeBuffer32[7] = boost_control_get_zero_point_duty_scaled();
	writeBuffer[32] = display_max_brightness;
	writeBuffer[33] = display_min_brightness;

	// Calculate byte wise checksum.
	uint32_t checksum = 0;

	for(int index = 4; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
	{
		checksum += writeBuffer[index];
	}

	writeBuffer32[0] = checksum;

	// Write page to EEPROM.
	bool verified = eeprom_24CS256 -> writePage(0, writeBuffer);

	if(verified)
	{
		// Verify written data.
		eeprom_24CS256 -> readPage(0, readBuffer);

		for(int index = 0; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
		{
			if(readBuffer[index] != writeBuffer[index])
			{
				verified = false;
				printf("Boost options commit failed verify at index %u of EEPROM page.\n", index);
				break;
			}
		}
	}

	return verified;
}

bool boost_options_read()
{
	uint8_t buffer[OPTIONS_EEPROM_PAGE_SIZE];

	bool okay = eeprom_24CS256 -> readPage(0, buffer);

	if(okay)
	{
		uint32_t* buffer32 = (uint32_t*) buffer;

		// Calculate byte wise checksum and compare.
		uint32_t checksum = 0;

		for(int index = 4; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
		{
			checksum += buffer[index];
		}

		okay = (buffer32[0] == checksum);

		if(okay)
		{
			boost_control_set_max_kpa_scaled(buffer32[1]);
			boost_control_set_de_energise_kpa_scaled(buffer32[2]);
			boost_control_set_pid_prop_const_scaled(buffer32[3]);
			boost_control_set_pid_integ_const_scaled(buffer32[4]);
			boost_control_set_pid_deriv_const_scaled(buffer32[5]);
			boost_control_set_max_duty_scaled(buffer32[6]);
			boost_control_set_zero_point_duty_scaled(buffer32[7]);
			display_max_brightness = buffer[32];
			display_min_brightness = buffer[33];
		}
		else
		{
			printf("Options read checksum failed. Could be bad EEPROM.\n");
		}
	}

	return okay;
}

void __testEeprom()
{
	uint32_t readBuffer[OPTIONS_EEPROM_PAGE_SIZE / 4];
	uint32_t writeBuffer[OPTIONS_EEPROM_PAGE_SIZE / 4];

	for(int index = 0; index < OPTIONS_EEPROM_PAGE_SIZE / 4; index++)
	{
		readBuffer[index] = 0;
		writeBuffer[index] = get_rand_32();
	}

	// Write random bytes to a range at the end of the EEPROM so that it crosses a 64 byte page boundary.
	// ie The lowest order 6 bits are the address within a page.

	uint32_t eepromAddr = 0xFFBB;

	bool allGood = eeprom_24CS256 -> writeBytes(eepromAddr, (uint8_t*)writeBuffer, 12);

	if(allGood)
	{
		// Read the bytes back and compare.
		allGood = eeprom_24CS256 -> readBytes(eepromAddr, (uint8_t*)readBuffer, 12);
	}

	if(allGood)
	{
		for(int index = 0; index < 3; index++)
		{
			if(writeBuffer[index] != readBuffer[index])
			{
				allGood = false;
				break;
			}
		}
	}

	if(!allGood)
	{
		printf("EEPROM non-page write/read test failed.\n");
	}
	else
	{
		printf("EEPROM non-page write/read test passed.\n");
	}

	// Ask EEPROM to read and verify its metadata.
	allGood = eeprom_24CS256 -> verifyMetaData(eepromPages, 1);

	if(!allGood)
	{
		printf("EEPROM meta data test failed.\n");
	}
	else
	{
		printf("EEPROM meta data test passed.\n");
	}

	// Test writing random page data.
	allGood = eeprom_24CS256 -> writePage(0, (uint8_t*)writeBuffer);

	if(allGood)
	{
		allGood = eeprom_24CS256 -> readPage(0, (uint8_t*)readBuffer);

		if(allGood)
		{
			for(int index = 0; index < OPTIONS_EEPROM_PAGE_SIZE / 4; index++)
			{
				if(readBuffer[index] != writeBuffer[index])
				{
					allGood = false;
					break;
				}
			}
		}
	}

	if(!allGood)
	{
		printf("EEPROM random page write test failed.\n");
	}
	else
	{
		printf("EEPROM random page write test passed.\n");
	}

	// Test commit of actual options data.
	allGood = boost_options_commit();

	if(!allGood)
	{
		printf("EEPROM commit options test failed.\n");
	}
	else
	{
		printf("EEPROM commit options test passed.\n");
	}

	// Test read of actual options data.
	allGood = boost_options_read();

	if(!allGood)
	{
		printf("EEPROM read options test failed.\n");
	}
	else
	{
		printf("EEPROM read options test passed.\n");
	}
}

void __runTests()
{
	// Allows testing equipment to detect test start.
	gpio_put(BOOST_OPTIONS_TEST_ACTIVE_GPIO, true);

	printf("Run tests starting.\n");

	printf("Map supply V: %.3f\n", boost_map_read_supply_voltage());
	printf("Map sensor V: %.3f\n", boost_map_read_sensor_voltage());

	//__testEeprom();

	printf("Testing solenoid valve.\n");
	boost_control_test_solenoid();

	printf("Run tests finished.\n");

	gpio_put(BOOST_OPTIONS_TEST_ACTIVE_GPIO, false);
}
