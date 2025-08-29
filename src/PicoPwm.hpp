#ifndef PICO_PWM_H
#define PICO_PWM_H

#include <stdint.h>

/**
 * Encapsulates the standard Pico PWM for a single slice only.
 * See the RP2040 datasheet for slice GPIO mappings.
 * @note This does _not_ check any given parameters for correctness against the slice.
 */
class PicoPwm
{
	public:

		virtual ~PicoPwm();

		/**
		 * @note PWM is initially disabled.
		 * @param chanAGpio The GPIO mapped to channel A of the PWM slice. Use -1 for not used.
		 *        This is _not_ checked for correctness.
		 * @param chanBGpio The GPIO mapped to channel B of the PWM slice. Use -1 for not used.
		 *        This is _not_ checked for correctness.
		 * @param initFreq Initial frequency of slice.
		 * @param initDutyCycleA Initial duty cycle for channel A.
		 * @param initDutyCycleB Initial duty cycle for channel B.
		 * @param phaseCorrect Whether to use Phase Correct mode.
		 * @param initDisableState The initial state passed to disable.
		 */
		PicoPwm(int chanAGpio, int chanBGpio, float initFreq, float initDutyCycleA, float initDutyCycleB, bool phaseCorrect,
			bool initDisableState);

		/** Get whether PWM is currently enabled. */
		bool getEnabled();

		/** Enable PWM. */
		void enable();

		/**
		 * Disable PWM.
		 * @param setHigh If true set the GPIO output(s) to high. Otherwise they will be set to low.
		 */
		void disable(bool setHigh);

		/** Set the slice frequency. */
		void setFreq(float freq);

		/**
		 * Get the current duty cycle for channel A.
		 * @returns Duty cycle, in %.
		 */
		float getDutyA();

		/**
		 * Get the current duty cycle for channel B.
		 * @returns Duty cycle, in %.
		 */
		float getDutyB();

		/**
		 * Set the duty cycle for one or both channels of the slice.
		 * @param dutyA Percentage duty cycle for channel A. A negative value means "Don't set".
		 * @param dutyB Percentage duty cycle for channel B. A negative value means "Don't set".
		 */
		void setDuty(float dutyA, float dutyB);

	private:

		/** Enable PWM. */
		void __enable();

		/**
		 * Disable PWM.
		 * @param setHigh If true set the GPIO output(s) to high. Otherwise they will be set to low.
		 */
		void __disable(bool setHigh);

		/**
		 * Set the slice frequency.
		 * @param freq Frequency in hz.
		 */
		void __setFreq(float freq);

		/**
		 * Set the duty cycle for one or both channels of the slice.
		 * @param dutyA Duty cycle for channel A, in %. A negative value means "Don't set".
		 * @param dutyB Duty cycle for channel B, in %. A negative value means "Don't set".
		 */
		void __setDuty(float dutyA, float dutyB);

		/** Whether PWM is currently enabled. */
		bool _enabled = false;

		/** Channel A GPIO number. -1 For not used. */
		int _chanAGpio;

		/** Channel B GPIO number. -1 For not used. */
		int _chanBGpio;

		/** The slice number that is being used. */
		int _sliceNumber;

		/**
		 * Current counter wrap value (TOP register).
		 * This is the width of the pulse in terms of the number of cycles coming out of clock divider.
		 * Phase correct being true effectively doubles this.
		 */
		uint16_t _counterWrap;

		/** Whether phase correct is being used. 0 for false, 1 for true. */
		uint8_t _phaseCorrect;

		/** The current duty cycle for channel A. */
		float _curDutyA;

		/** The current duty cycle for channel B. */
		float _curDutyB;
};

#endif