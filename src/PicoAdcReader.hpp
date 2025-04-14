#ifndef PICO_ADC_READER_H
#define PICO_ADC_READER_H

#include <stdint.h>

#include "AdcReader.hpp"

/**
 * On board Pi Pico ADC Reader.
 */
class PicoAdcReader : public AdcReader
{
	public:

		virtual ~PicoAdcReader();

		/**
		 * @param adcInput ADC input number.
		 * @param avgCount The number of read values to average the result over.
		 * @param vRef Voltage reference to ADC.
		 * @param scale Scale the read voltage by this amount to get the final voltage.
		 *        Supports voltage divided input to the ADC.
		 */
		PicoAdcReader(unsigned adcInput, unsigned avgCount, double vRef, double scale);

	protected:

		// Impl.
		uint32_t _readFromAdc(unsigned adcInput);

	private:
};

#endif