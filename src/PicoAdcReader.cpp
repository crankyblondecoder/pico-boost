#include "hardware/adc.h"

#include "PicoAdcReader.hpp"

PicoAdcReader::~PicoAdcReader()
{
}

PicoAdcReader::PicoAdcReader(unsigned adcInput, unsigned avgCount, double vRef, double scale) :
	AdcReader(adcInput, avgCount, vRef, 12, scale)
{
	unsigned gpioPin = adcInput + 26;

	// This should make the GPIO high-impedance.
    adc_gpio_init(gpioPin);
}

uint32_t PicoAdcReader::_readFromAdc(unsigned adcInput)
{
	adc_select_input(adcInput);
	return adc_read();
}
