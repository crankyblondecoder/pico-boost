#ifndef PICO_SWITCH_H
#define PICO_SWITCH_H

#include <stdint.h>

#include "pico/time.h"

/**
 * Generic switch.
 * @note Does debounce automatically.
 */
class PicoSwitch
{
	public:

		/** Indiciates whether pull up or down resistor is enabled. */
		enum PullUpDown { NONE, PULL_UP, PULL_DOWN };

		virtual ~PicoSwitch();

		/**
		 * @param gpio GPIO to read switch from.
		 * @param pullUpDown Whether pull up/down resistor is enabled. If a pull up is enabled it is assumed the switch
		 *        being pressed pulls the GPIO pin low.
		 * @param activeCountThreshold The switch pin active count at which counting stops. This provides hysteresis.
		 * @param sampleDuration Minimum duration, in microseconds, between reading the GPIO.
		 */
		PicoSwitch(unsigned gpio, PullUpDown pullUpDown, unsigned activeCountThreshold, unsigned sampleDuration);

		/** Poll the GPIO for switch state. */
		void poll();

		/**
		 * Get the state of the switch.
		 * @returns True for switch pressed. False for not pressed.
		 */
		bool getSwitchState();

		/**
		 * Get the amount of time the switch has stayed in its current state. In milliseconds.
		 */
		unsigned getSwitchStateDuration();

		/**
		 * Get the current state cycle index. This is the index of the switch press as the start of the switch
		 * press/release cycle.
		 * @returns Cycle index number. 0 indicates that no cycle has begun yet.
		 */
		unsigned getCurrentStateCycleIndex();

		/**
		 * Get the duration of the last complete switch press, in milliseconds.
		 * ie This does not cover any currently incomplete switch press state.
		 */
		unsigned getLastPressDuration();

	private:

		/** GPIO pin assigned to switch. */
		unsigned _gpio;

		/**
		 * Whether pull up/down resistor is enabled. If a pull up is enabled it is assumed the switch being pressed
		 * pulls the GPIO pin low.
		 */
		PullUpDown _pullUpDown;

		/** Current count of number of times the switch pin is sampled as active. Provides hysteresis. */
		unsigned _activeCount = 0;

		/** The switch pin active count at which counting stops. */
		unsigned _activeCountThreshold;

		/** Minimum duration, in microseconds, between reading the GPIO. */
		unsigned _sampleDuration = 0;

		/** The time of the last sample. */
		absolute_time_t _lastSampleTime;

		/** Current switch state. True for pressed, false for not pressed (released). */
		bool _currentState = false;

		/** The time the current switch (pressed/released) state became definite. */
		absolute_time_t _curStateTime;

		/** Counter used to indicate a unique instance of the the start of the "leading edge" of a switch press. */
		unsigned _stateCycleCounter = 0;

		/** The duration of the last switch press, in milliseconds. Does not contain the duration of the current state. */
		unsigned _lastPressDuration = 0;
};

#endif