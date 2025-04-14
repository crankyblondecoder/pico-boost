#ifndef ADC_READER_H
#define ADC_READER_H

#include <stdint.h>

/**
 * Base of all Analog to Digital reading.
 */
class AdcReader
{
	public:

		virtual ~AdcReader();

		/**
		 * @param adcInput ADC input number.
		 * @param avgCount The number of read values to average the result over.
		 * @param vRef Voltage reference to ADC.
		 * @param adcResolution Number of bits resolution the ADC provides.
		 * @param scale Scale the read voltage by this amount to get the final voltage.
		 *        Supports voltage divided input to the ADC.
		 */
		AdcReader(unsigned adcInput, unsigned avgCount, double vRef, unsigned adcResolution, double scale);

		/**
		 * Read the ADC.
		 */
		void latch();

		/**
		 * Return the raw averaged ADC value.
		 * @note This has _not_ been converted to a voltage.
		 */
		uint32_t readRaw();

		/**
		 * Read the averaged ADC voltage.
		 */
		double read();

	protected:

		/** Read raw ADC input value. */
		virtual uint32_t _readFromAdc(unsigned adcInput) = 0;

	private:

		/**
		 * Calculate the ADC averge values across all raw latched data.
		 */
		uint32_t __calcRawAvgVals();

		/**
		 * Total number of raw ADC values averaged over.
		 */
		unsigned _avgCount;

		/** Raw ADC values that have been latched. */
		uint32_t* _rawVals;

		/** The current position that raw vals are written into. */
		unsigned _curRawValPosn = 0;

		/** ADC Input to read from. */
		unsigned _adcInput;

		/** Scale applied to ADC output to get voltage. */
		double _voltageScale;
};

#endif