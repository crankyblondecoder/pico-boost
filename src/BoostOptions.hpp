#ifndef BOOST_OPTIONS_H
#define BOOST_OPTIONS_H

#include "BoostControl.hpp"
#include "BoostControlParameters.hpp"
#include "PicoSwitch.hpp"
#include "TM1637_pico.hpp"
#include "Eeprom_24CS256.hpp"

#include "gpioAlloc.hpp"

// Display character mapping
// -------------------------
// E: Default display energised.
// L: Default display max boost reached.
// C: Current solenoid control valve duty cycle.
// T: CURRENT_PRESET_INDEX
// B: BOOST_MAX_KPA
// U: BOOST_DE_ENERGISE_KPA
// A: BOOST_PID_ACTIVE_KPA
// P: BOOST_PID_PROP_CONST
// J: BOOST_PID_INTEG_CONST
// D: BOOST_PID_DERIV_CONST
// Q: BOOST_MAX_DUTY
// O: BOOST_ZERO_POINT_DUTY
// R: DISPLAY_MAX_BRIGHTNESS
// H: DISPLAY_MIN_BRIGHTNESS
// Y: FACTORY_RESET

/** The GPIO pin that is set high to indicate that boost options initiated testing is active. */
#define BOOST_OPTIONS_TEST_ACTIVE_GPIO 3

/** Size of EEPROM page, in bytes, that stores options. */
#define OPTIONS_EEPROM_PAGE_SIZE 192

/**
 * Boost option processing.
 * Controls the display and button input.
 * @note This is currently intended to be a singleton.
 */
class BoostOptions
{
	public:

		virtual ~BoostOptions();

		BoostOptions();

		/** Select display value options. */
		enum SelectOption
		{
			/** Current boost as read by map sensor. PSI Output. */
			CURRENT_BOOST_PSI,

			/** Current boost as read by map sensor. KPA Output. */
			CURRENT_BOOST_KPA,

			/**
			 * Current duty cycle being appied to solenoid valve.
			 * @note Always keep this second. If it has to change, alter the edit mode entry if statement.
			 */
			CURRENT_DUTY,

			/** Current boost parameters preset index. */
			CURRENT_PRESET_INDEX,

			/** Maximum boost, in kPa. */
			BOOST_MAX_KPA,

			/** Boost value, in kPa, below which the wastegate solenoid is de-energised. */
			BOOST_DE_ENERGISE_KPA,

			/** Boost value, in Kpa, above which the PID algorithm is used to control boost. */
			BOOST_PID_ACTIVE_KPA,

			/** Boost PID control, proportional constant. */
			BOOST_PID_PROP_CONST,

			/** Boost PID control, integration constant. */
			BOOST_PID_INTEG_CONST,

			/** Boost PID control, derivative constant. */
			BOOST_PID_DERIV_CONST,

			/** Boost control solenoid maximum duty cycle. In %. */
			BOOST_MAX_DUTY,

			/** Boost control solenoid zero point duty cycle. In %. */
			BOOST_ZERO_POINT_DUTY,

			/** Maximum brightness of display. */
			DISPLAY_MAX_BRIGHTNESS,

			/** Minimum brightness of display. (ie "Headlights on")*/
			DISPLAY_MIN_BRIGHTNESS,

			/** Factory reset. Set all data back to original defaults. */
			FACTORY_RESET,

			/** Not an option, just used to indicate enum length. */
			SELECT_OPTION_LAST
		};

		/**
		 * Give this object a slice of cpu time.
		 */
		void poll();

	protected:

	private:

		/** Single boost control instance. */
		BoostControl _boostControl;

		/** Button for select/up operations. */
		PicoSwitch* _selectUpButton;

		/** The last _fully_ procesed select/up button state index. */
		unsigned _lastProcSelectUpButtonStateIndex = 0;

		/** Button for down operation. */
		PicoSwitch* _downButton;

		/** The last _fully_ processed down button state index. */
		unsigned _lastProcDownButtonStateIndex = 0;

		/** Detect gpio being asserted for minimum display brightness as a switch. */
		PicoSwitch* _minBrightnessInput;

		/** 4 Digit display. */
		TM1637Display* _display;

		/** Displays maximum brightness. 0-7. */
		uint8_t _displayMaxBrightness = 7;

		/** Displays minimum brightness. 0-7. */
		uint8_t _displayMinBrightness = 4;

		/** Whether to use minimum brightness for display. */
		bool _displayUseMinBrightness = false;

		/** Data transfered to display. */
		uint8_t _dispData[4];

		/** Next render time for display. Stops flickering of the display when values change rapidly. */
		absolute_time_t _nextDisplayRenderTime;

		/** The default selected option. */
		int _defaultSelectOption = CURRENT_BOOST_PSI;

		/** Currently selected option. */
		int _curSelectedOption;

		/** Whether edit mode is active. */
		bool _editMode = false;

		/** During edit mode fast mode, the last up switch duration that was acted upon. */
		unsigned _lastEditFastModeUpDuration = 0;

		/** During edit mode fast mode, the last down switch duration that was acted upon. */
		unsigned _lastEditFastModeDownDuration = 0;

		/** Flag to indicate whether display is on in the display "flashing" cycle. */
		bool _displayFlashOn = true;

		/** Next absolute time to toggle the current display flash flag. */
		absolute_time_t _nextDisplayFlashToggleTime = 0;

		/** 24CS256 EEPROM responding to address 0 on i2c bus 0. */
		Eeprom_24CS256* _eeprom24CS256;

		EepromPage _eepromPages[1] = {{OPTIONS_EEPROM_PAGE_SIZE, 64}};

		/** Boost presets. */
		BoostControlParameters _boostPresets[5];

		/** Index of the current boost preset being used. */
		int _curBoostPresetIndex = 0;

		/** Run options related tests. */
		void __runTests();

		/** Display the current boost, in kPa. */
		void __displayCurrentBoostKpa();

		/** Display the current boost, in psi */
		void __displayCurrentBoostPsi();

		/** Display the current duty cycle. */
		void __displayCurrentDuty();

		/** Display the current maximum boost. */
		void __displayMaxBoost();

		/** Display the current de-energised boost level. */
		void __displayBoostDeEnergise();

		/** Display the boost level when the PID algorithm becomes active. */
		void __displayBoostPidActive();

		/** Display the PID algorithm proportional constant. */
		void __displayBoostPidPropConst();

		/** Display the PID algorithm integral constant. */
		void __displayBoostPidIntegConst();

		/** Display the PID algorithm derivative constant. */
		void __displayBoostPidDerivConst();

		/** Display the maximum duty cycle that can be used. In percent. */
		void __displayBoostMaxDuty();

		/** Display the duty cycle that, for a steady state, will result in the required max boost. */
		void __displayBoostZeroPointDuty();

		/** Display the maximum display brightness. */
		void __displayShowMaxBrightness();

		/** Display the maximum display brightness. */
		void __displayShowMinBrightness();

		/** Display the factory reset indicator. */
		void __displayShowFactoryReset();

		/** Display the current boost preset index. */
		void __displayCurrentPresetIndex();

		/** Invoke the factory reset. */
		void __invokeFactoryReset();

		/** Set all variables to default values. */
		void __setDefaults();

		/** Setup boost parameters from the current preset. */
		void __setupFromCurPreset();

		/**
		 * Commit the current boost options to EEPROM.
		 * @returns True if commit was verified as being successful.
		 */
		bool __commitToEeprom();

		/**
		 * Read the current boost options from EEPROM and set on other modules as appropriate.
		 * @returns True if read was successful.
		 */
		bool __readFromEeprom();

		/**
		 * Alter the current preset index by the given delta.
		 */
		void __alterCurPresetIndex(int delta);
};

#endif