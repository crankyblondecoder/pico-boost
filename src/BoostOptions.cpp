#include <stdio.h>

#include "BoostOptions.hpp"

extern bool debug;

/** Amount of time that both buttons have to be pressed to invoke a test pass. */
#define TEST_START_TIMEOUT 5000

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

BoostOptions::~BoostOptions()
{
	if(_eeprom24CS256) delete _eeprom24CS256;
	if(_display) delete _display;
	if(_selectUpButton) delete _selectUpButton;
	if(_downButton) delete _downButton;
	if(_minBrightnessInput) delete _minBrightnessInput;
}

BoostOptions::BoostOptions()
{
	_curSelectedOption = _defaultSelectOption;

	// Create EEPROM instance and initialise it.
	// Current Use wear levelled page of size 32.
	// Current saved boost options size: 24

	_eeprom24CS256 = new Eeprom_24CS256(i2c0, 0, _eepromPages, 1);

	__setDefaults();

	// Read initial options.
	__readFromEeprom();

	// Create 4 digit display instance.
	_display = new TM1637Display(DISPLAY_CLOCK_GPIO, DISPLAY_DATA_GPIO);

	// Initial display data.
	_dispData[3] = _display -> encodeDigit(0);
	_dispData[2] = 0;
	_dispData[1] = 0;
	_dispData[0] = 0;

	_display -> show(_dispData);

	_display -> setBrightness(_displayMaxBrightness);

	// Up/Select mode button.
	_selectUpButton = new PicoSwitch(UP_SELECT_BUTTON_GPIO, PicoSwitch::PULL_UP, 5, 100);

	// Down button.
	_downButton = new PicoSwitch(DOWN_BUTTON_GPIO, PicoSwitch::PULL_UP, 5, 100);

	// Min brightness.
	_minBrightnessInput = new PicoSwitch(MIN_BRIGHTNESS_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	_nextDisplayRenderTime = get_absolute_time();

	_nextDisplayFlashToggleTime = _nextDisplayRenderTime;
}

void BoostOptions::poll()
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

			case CURRENT_PRESET_INDEX:

				display_current_preset_index();
				break;

			case BOOST_MAX_KPA:

				display_max_boost();
				break;

			case BOOST_DE_ENERGISE_KPA:

				display_boost_de_energise();
				break;

			case BOOST_PID_ACTIVE_KPA:

				display_boost_pid_active();
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

			case FACTORY_RESET:

				display_show_factory_reset();
				break;
		}
	}
}

void BoostOptions::__displayCurrentBoostKpa()
{
	int boostKpaScaled = _boostControl.getKpaScaled();

	// Show kpa with 0 decimal points.
	int dispKpa = boostKpaScaled / 1000;
	if(dispKpa < 0) dispKpa *= -1;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(_boostControl.isMaxBoostReached())
	{
		_dispData[0] = _display -> encodeAlpha('L');
	}
	else if(_boostControl.isEnergised())
	{
		_dispData[0] = _display -> encodeAlpha('E');
	}
	else
	{
		// Accounts for flutter around zero. ie Stops negative sign from flashing randomly.
		if(boostKpaScaled <= -1000)
		{
			_dispData[0] = _display -> encodeAlpha('-');
		}
		else
		{
			_dispData[0] = 0;
		}
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayCurrentBoostPsi()
{
	int boostPsiScaled = _boostControl.getPsiScaled();

	// Show psi with 0 decimal points.
	int dispPsi = boostPsiScaled / 10;
	if(dispPsi < 0) dispPsi *= -1;

	// Display just 2 digits of psi value.
	_display -> encodeNumber(dispPsi, 2, 3, _dispData);

	// Default to nothing in the left most char for now.
	_dispData[0] = 0;

	if(_boostControl.isMaxBoostReached())
	{
		_dispData[0] = _display -> encodeAlpha('L');
	}
	else if(_boostControl.isEnergised())
	{
		_dispData[0] = _display -> encodeAlpha('E');
	}
	else
	{
		// Accounts for flutter around zero. ie Stops negative sign from flashing randomly.
		if(boostPsiScaled <= -10)
		{
			_dispData[1] = _display -> encodeAlpha('-');
		}
		else
		{
			_dispData[1] = 0;
		}
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayCurrentDuty()
{
	unsigned curDuty = _boostControl.getCurrentDutyScaled();

	unsigned dispDuty = curDuty / 10;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispDuty, 3, 3, _dispData);

	_dispData[0] = _display -> encodeAlpha('C');

	_display -> show(_dispData);
}

void BoostOptions::__displayMaxBoost()
{
	unsigned boost_max_kpa_scaled = _boostControl.getMaxKpaScaled();

	// Show max kpa with 0 decimal points.
	unsigned dispKpa = boost_max_kpa_scaled / 1000;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('B');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostDeEnergise()
{
	unsigned boostDeEnergisedScaled = _boostControl.getDeEnergiseKpaScaled();

	// Show with 0 decimal points.
	unsigned dispKpa = boostDeEnergisedScaled / 1000;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('U');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostPidActive()
{
	unsigned boost_pid_active_kpa_scaled = _boostControl.getPidActiveKpaScaled();

	// Show with 0 decimal points.
	unsigned dispKpa = boost_pid_active_kpa_scaled / 1000;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('A');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostPidPropConst()
{
	unsigned boostPidPropScaled = _boostControl.getPidPropConstScaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boostPidPropScaled / 10;

	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('P');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostPidIntegConst()
{
	unsigned boost_pid_integ_scaled = _boostControl.getPidIntegConstScaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boost_pid_integ_scaled / 10;

	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('J');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostPidDerivConst()
{
	unsigned boost_pid_deriv_scaled = _boostControl.getPidDerivConstScaled();

	// Show with 2 decimal points.
	unsigned dispKpa = boost_pid_deriv_scaled / 10;

	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('D');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostMaxDuty()
{
	unsigned boost_max_duty = _boostControl.getMaxDutyScaled();

	// Show with 1 decimal point.
	unsigned dispVal = boost_max_duty;

	_display -> encodeNumber(dispVal, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('Q');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayBoostZeroPointDuty()
{
	unsigned boost_zero_point_duty = _boostControl.getZeroPointDutyScaled();

	// Show with 1 decimal point.
	unsigned dispVal = boost_zero_point_duty;

	_display -> encodeNumber(dispVal, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('O');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayShowMaxBrightness()
{
	// Show with 0 decimal points.
	unsigned dispVal = _displayMaxBrightness;

	_display -> encodeNumber(dispVal, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('R');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayShowMinBrightness()
{
	// Show with 0 decimal points.
	unsigned dispVal = _displayMinBrightness;

	_display -> encodeNumber(dispVal, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('H');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayShowFactoryReset()
{
	_dispData[1] = 0;
	_dispData[2] = 0;
	_dispData[3] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('Y');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayCurrentPresetIndex()
{
	// Note: Preset index is displayed from 1 -> n even though it is zero based.

	// Display just 1 digit.
	_display -> encodeNumber(_curBoostPresetIndex + 1, 3, 3, _dispData);

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('T');
	}
	else
	{
		_dispData[0] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__invokeFactoryReset()
{
	__setDefaults();

	// Save options to EEPROM.
	__commitToEeprom();
}

void BoostOptions::__setDefaults()
{
	// Initialise boost presets to defaults.
	_boostControl.populateDefaultParameters(_boostPresets);
	_boostControl.populateDefaultParameters(_boostPresets + 1);
	_boostControl.populateDefaultParameters(_boostPresets + 2);
	_boostControl.populateDefaultParameters(_boostPresets + 3);
	_boostControl.populateDefaultParameters(_boostPresets + 4);

	_curBoostPresetIndex = 0;

	__setupFromCurPreset();

	_displayMaxBrightness = 7;
	_displayMinBrightness = 4;
	_displayUseMinBrightness = false;

	_editMode = false;

	_curSelectedOption = _defaultSelectOption;

	_displayFlashOn = true;
}

void BoostOptions::__setupFromCurPreset()
{
	_boostControl.populateDefaultParameters(_boostPresets + _curBoostPresetIndex);
}

bool BoostOptions::__commitToEeprom()
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
	int index32 = 1;

	// Five presets.
	for(int index = 0; index < 5; index++)
	{
		BoostControlParameters*  curBoostPresets = _boostPresets + index;

		writeBuffer32[index32++] = curBoostPresets -> maxKpaScaled;
		writeBuffer32[index32++] = curBoostPresets -> deEnergiseKpaScaled;
		writeBuffer32[index32++] = curBoostPresets -> pidActiveKpaScaled;
		writeBuffer32[index32++] = curBoostPresets -> pidPropConstScaled;
		writeBuffer32[index32++] = curBoostPresets -> pidIntegConstScaled;
		writeBuffer32[index32++] = curBoostPresets -> pidDerivConstScaled;
		writeBuffer32[index32++] = curBoostPresets -> maxDuty;
		writeBuffer32[index32++] = curBoostPresets -> zeroPointDuty;
	}

	int index8 = (index32 - 1) * 4;

	writeBuffer[index8++] = _displayMaxBrightness;
	writeBuffer[index8++] = _displayMinBrightness;
	writeBuffer[index8++] = _curBoostPresetIndex;

	// Calculate byte wise checksum.
	uint32_t checksum = 0;

	for(int index = 4; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
	{
		checksum += writeBuffer[index];
	}

	writeBuffer32[0] = checksum;

	// Write page to EEPROM.
	bool verified = _eeprom24CS256 -> writePage(0, writeBuffer);

	if(verified)
	{
		// Verify written data.
		_eeprom24CS256 -> readPage(0, readBuffer);

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

bool BoostOptions::__readFromEeprom()
{
	uint8_t readBuffer[OPTIONS_EEPROM_PAGE_SIZE];

	bool okay = _eeprom24CS256 -> readPage(0, readBuffer);

	if(okay)
	{
		uint32_t* readBuffer32 = (uint32_t*) readBuffer;

		// Calculate byte wise checksum and compare.
		uint32_t checksum = 0;

		for(int index = 4; index < OPTIONS_EEPROM_PAGE_SIZE; index++)
		{
			checksum += readBuffer[index];
		}

		okay = (readBuffer32[0] == checksum);

		if(okay)
		{
			// Remember that the first entry in the buffer is the checksum.
			int index32 = 1;

			// Five presets.
			for(int index = 0; index < 5; index++)
			{
				BoostControlParameters*  curBoostPresets = _boostPresets + index;

				curBoostPresets -> maxKpaScaled = readBuffer32[index32++];
				curBoostPresets -> deEnergiseKpaScaled = readBuffer32[index32++];
				curBoostPresets -> pidActiveKpaScaled = readBuffer32[index32++];
				curBoostPresets -> pidPropConstScaled = readBuffer32[index32++];
				curBoostPresets -> pidIntegConstScaled = readBuffer32[index32++];
				curBoostPresets -> pidDerivConstScaled = readBuffer32[index32++];
				curBoostPresets -> maxDuty = readBuffer32[index32++];
				curBoostPresets -> zeroPointDuty = readBuffer32[index32++];
			}

			int index8 = (index32 - 1) * 4;

			_displayMaxBrightness = readBuffer[index8++];
			_displayMinBrightness = readBuffer[index8++];
			_curBoostPresetIndex = readBuffer[index8++];

			// Set the params on control only after the preset index is read.
			__setupFromCurPreset();
		}
		else
		{
			printf("Options read checksum failed. Could be bad EEPROM.\n");
		}
	}

	return okay;
}

void BoostOptions::__alterCurPresetIndex(int delta)
{
	int newPresetIndex = _curBoostPresetIndex + delta;

	if(newPresetIndex < 0) newPresetIndex = 4;

	if(newPresetIndex > 4) newPresetIndex = 0;

	if(_curBoostPresetIndex != newPresetIndex)
	{
		_curBoostPresetIndex = newPresetIndex;

		__setupFromCurPreset();
	}
}