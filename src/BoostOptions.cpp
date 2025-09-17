#include <stdio.h>

#include "hardware/gpio.h"

#include "BoostOptions.hpp"

extern bool debug;

/** Amount of time that button(s) have to be pressed to invoke a test pass. */
#define TEST_START_TIMEOUT 10000

/** Amount of time of inactivity, in milliseconds, before the current mode ends and the system returns to default. */
#define MODE_COMPLETE_TIMEOUT 5000

/** Amount of time, in milliseconds, that the select button must be pressed to enter edit mode. */
#define MODE_ENTER_EDIT_TIME 2500

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

	if(_selectButton) delete _selectButton;
	if(_leftButton) delete _leftButton;
	if(_rightButton) delete _rightButton;
	if(_increaseButton) delete _increaseButton;
	if(_decreaseButton) delete _decreaseButton;

	if(_minBrightnessInput) delete _minBrightnessInput;

	if(_presetSelectInput) delete _presetSelectInput;
}

BoostOptions::BoostOptions(BoostControl* boostControl)
{
	_boostControl = boostControl;

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

	// Setup navigation buttons.

	_selectButton = new PicoSwitch(NAV_BTN_MIDDLE, PicoSwitch::PULL_UP, 5, 100);

	_leftButton = new PicoSwitch(NAV_BTN_LEFT, PicoSwitch::PULL_UP, 5, 100);

	_rightButton = new PicoSwitch(NAV_BTN_RIGHT, PicoSwitch::PULL_UP, 5, 100);

	_increaseButton = new PicoSwitch(NAV_BTN_FORWARD, PicoSwitch::PULL_UP, 5, 100);

	_decreaseButton = new PicoSwitch(NAV_BTN_BACK, PicoSwitch::PULL_UP, 5, 100);

	// Min brightness.
	_minBrightnessInput = new PicoSwitch(MIN_BRIGHTNESS_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	// Pre-defined preset index select.
	_presetSelectInput = new PicoSwitch(PRESET_INDEX_SELECT_GPIO, PicoSwitch::PULL_DOWN, 5, 100);

	_nextDisplayRenderTime = get_absolute_time();

	_nextDisplayFlashToggleTime = _nextDisplayRenderTime;
}

void BoostOptions::poll()
{
	// Note: Make sure the polling frequency is high enough that switches can debounce.
	_selectButton -> poll();
	_leftButton -> poll();
	_rightButton -> poll();
	_increaseButton -> poll();
	_decreaseButton -> poll();
	_minBrightnessInput -> poll();
	_presetSelectInput -> poll();

	_displayUseMinBrightness = _minBrightnessInput -> getSwitchState();

	__processSwitches();

	// Normal non-options display is active.
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= _nextDisplayRenderTime)
	{
		// Set brightness of display.
		if(_displayUseMinBrightness)
		{
			_display -> setBrightness(_displayMinBrightness);
		}
		else
		{
			_display -> setBrightness(_displayMaxBrightness);
		}

		// Process current display flashing toggle.
		if(curTime > _nextDisplayFlashToggleTime) {

			_displayFlashOn = !_displayFlashOn;

			_nextDisplayFlashToggleTime = delayed_by_ms(_nextDisplayFlashToggleTime, DISPLAY_FLASH_PERIOD);
		}

		// Limit the frame rate so the display doesn't "strobe".
		_nextDisplayRenderTime = delayed_by_ms(_nextDisplayRenderTime, DISPLAY_FRAME_RATE);

		switch(_curSelectedOption)
		{
			case CURRENT_BOOST_KPA:

				__displayCurrentBoostKpa();
				break;

			case CURRENT_BOOST_PSI:

				__displayCurrentBoostPsi();
				break;

			case CURRENT_DUTY:

				__displayCurrentDuty();
				break;

			case CURRENT_PRESET_INDEX:

				__displayPresetIndex();
				break;

			case PRESET_SELECT_INDEX:

				__displayPresetSelectIndex();
				break;

			case AUTO_TUNE:

				__displayAutoTune();
				break;

			case BOOST_MAX_KPA:

				__displayMaxBoost();
				break;

			case BOOST_DE_ENERGISE_KPA:

				__displayBoostDeEnergise();
				break;

			case BOOST_PID_ACTIVE_KPA:

				__displayBoostPidActive();
				break;

			case BOOST_PID_PROP_CONST:

				__displayBoostPidPropConst();
				break;

			case BOOST_PID_INTEG_CONST:

				__displayBoostPidIntegConst();
				break;

			case BOOST_PID_DERIV_CONST:

				__displayBoostPidDerivConst();
				break;

			case BOOST_MAX_DUTY:

				__displayBoostMaxDuty();
				break;

			case BOOST_ZERO_POINT_DUTY:

				__displayBoostZeroPointDuty();
				break;

			case DISPLAY_MAX_BRIGHTNESS:

				__displayMaxBrightness();
				break;

			case DISPLAY_MIN_BRIGHTNESS:

				__displayMinBrightness();
				break;

			case FACTORY_RESET:

				__displayFactoryReset();
				break;
		}
	}
}

void BoostOptions::__displayCurrentBoostKpa()
{
	int boostKpaScaled = _boostControl -> getKpaScaled();

	// Show kpa with 0 decimal points.
	int dispKpa = boostKpaScaled / 1000;
	if(dispKpa < 0) dispKpa *= -1;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispKpa, 3, 3, _dispData);

	if(_boostControl -> isMaxBoostReached())
	{
		_dispData[0] = _display -> encodeAlpha('L');
	}
	else if(_boostControl -> isEnergised())
	{
		_dispData[0] = _display -> encodeAlpha('E');
	}
	else if(_presetSelectIndexActive && boostKpaScaled > -1000)
	{
		// Don't display this indicator when a negative sign is required.
		// Can get away with this because displaying kPa is far less likely than psi.
		_dispData[0] = _display -> encodeAlpha('N');
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
	int boostPsiScaled = _boostControl -> getPsiScaled();

	// Show psi with 0 decimal points.
	int dispPsi = boostPsiScaled / 10;
	if(dispPsi < 0) dispPsi *= -1;

	// Display just 2 digits of psi value.
	_display -> encodeNumber(dispPsi, 2, 3, _dispData);

	// Default to nothing in the left most char.
	_dispData[0] = 0;

	if(_boostControl -> isMaxBoostReached())
	{
		_dispData[0] = _display -> encodeAlpha('L');
	}
	else if(_boostControl -> isEnergised())
	{
		_dispData[0] = _display -> encodeAlpha('E');
	}
	else if(_presetSelectIndexActive)
	{
		_dispData[0] = _display -> encodeAlpha('N');
	}

	// Possibly display negative symbol. Accounts for flutter around zero. ie Stops negative sign from flashing randomly.
	if(boostPsiScaled <= -10)
	{
		_dispData[1] = _display -> encodeAlpha('-');
	}
	else
	{
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayCurrentDuty()
{
	unsigned curDuty = _boostControl -> getCurrentDutyScaled();

	unsigned dispDuty = curDuty / 10;

	// Display just 3 digits of kPa value.
	_display -> encodeNumber(dispDuty, 3, 3, _dispData);

	_dispData[0] = _display -> encodeAlpha('C');

	_display -> show(_dispData);
}

void BoostOptions::__displayMaxBoost()
{
	unsigned boost_max_kpa_scaled = _boostControl -> getMaxKpaScaled();

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
	unsigned boostDeEnergisedScaled = _boostControl -> getDeEnergiseKpaScaled();

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
	unsigned boost_pid_active_kpa_scaled = _boostControl -> getPidActiveKpaScaled();

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
	unsigned boostPidPropScaled = _boostControl -> getPidPropConstScaled();

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
	unsigned boost_pid_integ_scaled = _boostControl -> getPidIntegConstScaled();

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
	unsigned boost_pid_deriv_scaled = _boostControl -> getPidDerivConstScaled();

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
	unsigned boost_max_duty = _boostControl -> getMaxDutyScaled();

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
	unsigned boost_zero_point_duty = _boostControl -> getZeroPointDutyScaled();

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

void BoostOptions::__displayMaxBrightness()
{
	// Show with 0 decimal points. Single digit only
	unsigned dispVal = _displayMaxBrightness;

	_display -> encodeNumber(dispVal, 1, 3, _dispData);

	_dispData[2] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('B');
		_dispData[1] = _display -> encodeAlpha('H');
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayMinBrightness()
{
	// Show with 0 decimal points. Single digit only.
	unsigned dispVal = _displayMinBrightness;

	_display -> encodeNumber(dispVal, 1, 3, _dispData);

	_dispData[2] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('B');
		_dispData[1] = _display -> encodeAlpha('L');
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayFactoryReset()
{
	_dispData[2] = 0;
	_dispData[3] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('F');
		_dispData[1] = _display -> encodeAlpha('R');
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayAutoTune()
{
	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('A');
		_dispData[1] = _display -> encodeAlpha('U');
		_dispData[2] = _display -> encodeAlpha('T');
		_dispData[3] = _display -> encodeAlpha('O');
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
		_dispData[2] = 0;
		_dispData[3] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayPresetIndex()
{
	// Note: Preset index is displayed from 1 -> n even though it is zero based.

	// Display just 1 digit.
	_display -> encodeNumber(_presetIndex + 1, 1, 3, _dispData);

	_dispData[2] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('B');
		_dispData[1] = _display -> encodeAlpha('N');

		if(!_presetSelectIndexActive && !_editMode && _displayFlashOn) _display -> encodeColon(_dispData + 1);
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__displayPresetSelectIndex()
{
	// Note: Preset select index is displayed from 1 -> n even though it is zero based.

	// Display just 1 digit.
	_display -> encodeNumber(_presetSelectIndex + 1, 1, 3, _dispData);

	_dispData[2] = 0;

	if(!_editMode || _displayFlashOn)
	{
		_dispData[0] = _display -> encodeAlpha('B');
		_dispData[1] = _display -> encodeAlpha('S');

		if(_presetSelectIndexActive && !_editMode && _displayFlashOn) _display -> encodeColon(_dispData + 1);
	}
	else
	{
		_dispData[0] = 0;
		_dispData[1] = 0;
	}

	_display -> show(_dispData);
}

void BoostOptions::__invokeFactoryReset()
{
	__setDefaults();

	// Save options to EEPROM.
	__commitToEeprom();
}

void BoostOptions::__invokeAutoTune()
{
	// TODO ...
}

void BoostOptions::__setDefaults()
{
	// Initialise boost presets to defaults.
	_boostControl -> populateDefaultParameters(_boostPresets);
	_boostControl -> populateDefaultParameters(_boostPresets + 1);
	_boostControl -> populateDefaultParameters(_boostPresets + 2);
	_boostControl -> populateDefaultParameters(_boostPresets + 3);
	_boostControl -> populateDefaultParameters(_boostPresets + 4);

	_presetIndex = 0;

	__setupControlFromCurPreset();

	_displayMaxBrightness = 7;
	_displayMinBrightness = 4;
	_displayUseMinBrightness = false;

	_editMode = false;

	_curSelectedOption = _defaultSelectOption;

	_displayFlashOn = true;
}

void BoostOptions::__setupControlFromCurPreset()
{
	// Takes into account whether preset select is active.

	if(_presetSelectIndexActive)
	{
		_boostControl -> setParameters(_boostPresets + _presetSelectIndex);
	}
	else
	{
		_boostControl -> setParameters(_boostPresets + _presetIndex);
	}
}

void BoostOptions::__populateCurPresetFromControl()
{
	// Takes into account whether preset select is active.

	if(_presetSelectIndexActive)
	{
		_boostControl -> getParameters(_boostPresets + _presetSelectIndex);
	}
	else
	{
		_boostControl -> getParameters(_boostPresets + _presetIndex);
	}
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

	// Make sure the local preset values are up to date.
	__populateCurPresetFromControl();

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

	// index32 is pointing to the start of the memory after the boost presets.
	int index8 = index32 * 4;

	writeBuffer[index8++] = _displayMaxBrightness;
	writeBuffer[index8++] = _displayMinBrightness;
	writeBuffer[index8++] = _presetIndex;
	writeBuffer[index8++] = _presetSelectIndex;

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

			// index32 is pointing to the start of the memory after the boost presets.
			int index8 = index32 * 4;

			_displayMaxBrightness = readBuffer[index8++];
			_displayMinBrightness = readBuffer[index8++];
			_presetIndex = readBuffer[index8++];
			_presetSelectIndex = readBuffer[index8++];

			// Set the params on control only after the preset index is read.
			__setupControlFromCurPreset();
		}
		else
		{
			printf("Options read checksum failed. Could be bad EEPROM.\n");
		}
	}

	return okay;
}

void BoostOptions::__alterPresetIndex(int delta)
{
	int newPresetIndex = _presetIndex + delta;

	if(newPresetIndex < 0) newPresetIndex = 4;

	if(newPresetIndex > 4) newPresetIndex = 0;

	if(_presetIndex != newPresetIndex)
	{
		// Save previous preset values if preset select isn't active.
		if(!_presetSelectIndexActive) __populateCurPresetFromControl();

		_presetIndex = newPresetIndex;

		if(!_presetSelectIndexActive) __setupControlFromCurPreset();
	}
}

void BoostOptions::__alterPresetSelectIndex(int delta)
{
	int newPresetSelectIndex = _presetSelectIndex + delta;

	if(newPresetSelectIndex < 0) newPresetSelectIndex = 4;

	if(newPresetSelectIndex > 4) newPresetSelectIndex = 0;

	if(_presetSelectIndex != newPresetSelectIndex)
	{
		// Save previous preset values if preset select is active.
		if(_presetSelectIndexActive) __populateCurPresetFromControl();

		_presetIndex = newPresetSelectIndex;

		if(_presetSelectIndexActive) __setupControlFromCurPreset();
	}
}

void BoostOptions::__processSwitches()
{
	unsigned curSelectStateIndex = _selectButton -> getCurrentStateCycleIndex();
	unsigned curLeftStateIndex = _leftButton -> getCurrentStateCycleIndex();
	unsigned curRightStateIndex = _rightButton -> getCurrentStateCycleIndex();
	unsigned curIncreaseStateIndex = _increaseButton -> getCurrentStateCycleIndex();
	unsigned curDecreaseStateIndex = _decreaseButton -> getCurrentStateCycleIndex();

	bool selectStateUnProc = curSelectStateIndex != _lastProcSelectButtonStateIndex;
	bool leftStateUnProc = curLeftStateIndex != _lastProcLeftButtonStateIndex;
	bool rightStateUnProc = curRightStateIndex != _lastProcRightButtonStateIndex;
	bool increaseStateUnProc = curIncreaseStateIndex != _lastProcIncreaseButtonStateIndex;
	bool decreaseStateUnProc = curDecreaseStateIndex != _lastProcDecreaseButtonStateIndex;

	// Whether a switch was processed.
	bool selectProced = false;
	bool leftProced = false;
	bool rightProced = false;
	bool increaseProced = false;
	bool decreaseProced = false;

	// Whether to committo eeprom.
	bool commitToEeprom = false;

	// Check for test invocation. Select pressed for an extended duration.
	if(selectStateUnProc && _selectButton -> getSwitchState() && _selectButton -> getSwitchStateDuration() > TEST_START_TIMEOUT)
	{
		__runTests();

		_lastProcSelectButtonStateIndex = curSelectStateIndex;
		_lastProcLeftButtonStateIndex = curLeftStateIndex;
		_lastProcRightButtonStateIndex = curRightStateIndex;
		_lastProcIncreaseButtonStateIndex = curIncreaseStateIndex;
		_lastProcDecreaseButtonStateIndex = curDecreaseStateIndex;

		return;
	}

	// Always process preset select.
	bool presetSelectIndexActive = _presetSelectInput -> getSwitchState();

	if(_presetSelectIndexActive != presetSelectIndexActive)
	{
		_presetSelectIndexActive = presetSelectIndexActive;

		// Force going out of edit mode because it might now be in an inconsistent state.
		_editMode = false;

		__setupControlFromCurPreset();
	}

	// Regardless of the current selected option, all switches not being pressed for greater than the mode complete timeout
	// puts the system back into a default mode. However solenoid duty cycle being displayed stays as is.
	if(!_selectButton -> getSwitchState() && _selectButton -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		!_leftButton -> getSwitchState() && _leftButton -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		!_rightButton -> getSwitchState() && _rightButton -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		!_increaseButton -> getSwitchState() && _increaseButton -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		!_decreaseButton -> getSwitchState() && _decreaseButton -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT &&
		_presetSelectInput -> getSwitchStateDuration() > MODE_COMPLETE_TIMEOUT)
	{
		if(_curSelectedOption != CURRENT_DUTY && _curSelectedOption != CURRENT_BOOST_PSI &&
			_curSelectedOption != CURRENT_BOOST_KPA)
		{
			_curSelectedOption = _defaultSelectOption;
		}

		if(_editMode)
		{
			// Edit mode is now cancelled.
			_editMode = false;

			// Discard any control changes.
			__setupControlFromCurPreset();
		}

		selectProced = true;
		leftProced = true;
		rightProced = true;
		increaseProced = true;
		decreaseProced = true;
	}
	else if(!_editMode)
	{
		// Remember that in non edit mode, all buttons only trigger something upon release. This is so that press and hold
		// actions can be detected.

		if(selectStateUnProc || leftStateUnProc || rightStateUnProc || increaseStateUnProc || decreaseStateUnProc)
		{
			bool proced = false;

			// Look for edit mode entry, which can't happen in either boost display modes or solenoid valve duty display mode.
			if(_curSelectedOption > CURRENT_DUTY && _selectButton -> getSwitchState() &&
				_selectButton -> getSwitchStateDuration() > MODE_ENTER_EDIT_TIME)
			{
				// Save current preset state locally so it can be restored if edit mode is cancelled.
				__populateCurPresetFromControl();

				_editMode = true;
				proced = true;
			}
			else if(increaseStateUnProc && !_increaseButton -> getSwitchState() && !_presetSelectIndexActive)
			{
				__alterPresetIndex(1);
				_curSelectedOption = CURRENT_PRESET_INDEX;

				commitToEeprom = true;
				proced = true;
			}
			else if(decreaseStateUnProc && !_decreaseButton -> getSwitchState() && !_presetSelectIndexActive)
			{
				__alterPresetIndex(-1);
				_curSelectedOption = CURRENT_PRESET_INDEX;

				commitToEeprom = true;
				proced = true;
			}
			else if(rightStateUnProc && !_rightButton -> getSwitchState())
			{
				// Change to the next state on right button _release_.
				_curSelectedOption++;

				if(_curSelectedOption >= SELECT_OPTION_LAST) _curSelectedOption = CURRENT_BOOST_PSI;

				proced = true;
			}
			else if(leftStateUnProc && !_leftButton -> getSwitchState())
			{
				// Change to the previous state on left button _release_.
				_curSelectedOption--;

				if(_curSelectedOption < 0) _curSelectedOption = SELECT_OPTION_LAST - 1;

				proced = true;
			}
			else if(selectStateUnProc)
			{
				// Select button state can be left unprocessed indefinitely if not cleared.
				if(!_selectButton -> getSwitchState()) proced = true;
			}

			if(proced)
			{
				// Generally assume all buttons are now processed.
				selectProced = true;
				leftProced = true;
				rightProced = true;
				increaseProced = true;
				decreaseProced = true;
			}
		}
	}
	else
	{
		// Should be in edit mode.

		// Select button exits edit mode.
		if(selectStateUnProc && _selectButton -> getSwitchState())
		{
			_editMode = false;

			// Basically reset all switch states.
			selectProced = true;
			leftProced = true;
			rightProced = true;
			increaseProced = true;
			decreaseProced = true;

			// If in factory reset mode, explicit exit of edit mode triggers the reset.
			if(_curSelectedOption == FACTORY_RESET) __invokeFactoryReset();

			// Auto tune is only triggered by edit mode exiting.
			if(_curSelectedOption == AUTO_TUNE) __invokeAutoTune();

			// Save options to EEPROM.
			commitToEeprom = true;
		}
		else
		{
			// Note: Non fast edit mode relies on button release to trigger change otherwise fast edit mode won't be detected
			// and edit mode exit will cause a simultaneous change in values which isn't desired.

			int delta = 0;

			if(increaseStateUnProc)
			{
				bool switchState = _increaseButton -> getSwitchState();

				unsigned increaseDuration = _increaseButton -> getSwitchStateDuration();

				if(switchState && increaseDuration > EDIT_MODE_FAST_TIME)
				{
					// Rate limit fast edit mode.
					if(increaseDuration - _lastEditFastModeIncreaseDuration > EDIT_MODE_FAST_REPEAT_RATE)
					{
						delta = 1;
						_lastEditFastModeIncreaseDuration = increaseDuration;
					}

					// Note: Button is left as unprocessed so this block can be re-entered.
				}
				else if(!switchState)
				{
					delta = 1;

					// Button is full processed.
					increaseProced = true;
				}
				else
				{
					// This should invoke fast edit change immediately.
					_lastEditFastModeIncreaseDuration = 0;
				}
			}
			else if(decreaseStateUnProc)
			{
				bool switchState = _decreaseButton -> getSwitchState();

				unsigned decreaseDuration = _decreaseButton -> getSwitchStateDuration();

				if(switchState && decreaseDuration > EDIT_MODE_FAST_TIME)
				{
					// Rate limit fast edit mode.
					if(decreaseDuration - _lastEditFastModeDecreaseDuration > EDIT_MODE_FAST_REPEAT_RATE)
					{
						delta = -1;
						_lastEditFastModeDecreaseDuration = decreaseDuration;
					}

					// Note: Button is left as unprocessed so this block can be re-entered.
				}
				else if(!switchState)
				{
					delta = -1;

					decreaseProced = true;
				}
				else
				{
					// This should invoke fast edit change immediately.
					_lastEditFastModeDecreaseDuration = 0;
				}
			}

			if(delta != 0)
			{
				switch(_curSelectedOption)
				{
					case CURRENT_PRESET_INDEX:

						__alterPresetIndex(delta);
						break;

					case PRESET_SELECT_INDEX:

						__alterPresetSelectIndex(delta);
						break;

					case BOOST_MAX_KPA:

						// Max kPa is scaled by 1000. Resolution 1.
						_boostControl -> alterMaxKpaScaled(delta * 1000);
						break;

					case BOOST_DE_ENERGISE_KPA:

						// De-energise is scaled by 1000. Resolution 1.
						_boostControl -> alterDeEnergiseKpaScaled(delta * 1000);
						break;

					case BOOST_PID_ACTIVE_KPA:

						// PID active is scaled by 1000. Resolution 1.
						_boostControl -> alterPidActiveKpaScaled(delta * 1000);
						break;

					case BOOST_PID_PROP_CONST:

						// PID proportional constant is scaled by 1000. Resolution 0.01.
						_boostControl -> alterPidPropConstScaled(delta * 10);
						break;

					case BOOST_PID_INTEG_CONST:

						// PID integration constant is scaled by 1000. Resolution 0.01.
						_boostControl -> alterPidIntegConstScaled(delta * 10);
						break;

					case BOOST_PID_DERIV_CONST:

						// PID derivative constant is scaled by 1000. Resolution 0.01.
						_boostControl -> alterPidDerivConstScaled(delta * 10);
						break;

					case BOOST_MAX_DUTY:

						// Boost solenoid maximum duty cycle is scaled by 10. Resolution 0.1.
						_boostControl -> alterMaxDutyScaled(delta);
						break;

					case BOOST_ZERO_POINT_DUTY:

						// Boost solenoid zero point duty cycle is scaled by 10. Resolution 0.1.
						_boostControl -> alterZeroPointDutyScaled(delta);
						break;

					case DISPLAY_MAX_BRIGHTNESS:

						// Maximum brightness 0-7.
						_displayMaxBrightness += delta;
						if(_displayMaxBrightness > 7) _displayMaxBrightness = 7;
						break;

					case DISPLAY_MIN_BRIGHTNESS:

						// Minimum brightness 0-7.
						_displayMinBrightness += delta;
						if(_displayMinBrightness > 7) _displayMinBrightness = 7;
						break;
				}
			}
		}
	}

	// Save the indexes of any fully processed buttons.
	if(selectProced) _lastProcSelectButtonStateIndex = curSelectStateIndex;
	if(leftProced) _lastProcLeftButtonStateIndex = curLeftStateIndex;
	if(rightProced) _lastProcRightButtonStateIndex = curRightStateIndex;
	if(increaseProced) _lastProcIncreaseButtonStateIndex = curIncreaseStateIndex;
	if(decreaseProced) _lastProcDecreaseButtonStateIndex = curDecreaseStateIndex;

	if(commitToEeprom) __commitToEeprom();
}

void BoostOptions::__runTests()
{
	// Show "test" on display.
	_dispData[0] = _display -> encodeAlpha('T');
	_dispData[1] = _display -> encodeAlpha('E');
	_dispData[2] = _display -> encodeAlpha('S');
	_dispData[3] = _display -> encodeAlpha('T');

	_display -> show(_dispData);

	// Allows testing equipment to detect test start.
	gpio_put(BOOST_OPTIONS_TEST_ACTIVE_GPIO, true);

	printf("Run tests starting.\n");

	printf("Map supply V: %.3f\n", _boostControl -> mapReadSupplyVoltage());
	printf("Map sensor V: %.3f\n", _boostControl -> mapReadSensorVoltage());

	//__testEeprom();

	printf("Testing solenoid valve.\n");
	_boostControl -> testSolenoid();

	printf("Run tests finished.\n");

	gpio_put(BOOST_OPTIONS_TEST_ACTIVE_GPIO, false);
}
