#include <stdio.h>

#include "hardware/adc.h"

#include "bosch_map_0261230119.hpp"

bool bosch_map_0261230119::_vSysRef_setup = false;

// If VRef is sourced from the power supply, the Pico has a ~30mV offset from the nominal 3.3V power supply.
double bosch_map_0261230119::_adcScale = 3.27 / (double)(1 << 12);

bosch_map_0261230119::bosch_map_0261230119(unsigned adcInput) : _adcInput(adcInput)
{
	unsigned GPIO_PIN = adcInput + 26;

	// This should make the GPIO high-impedance.
    adc_gpio_init(GPIO_PIN);

	// Setup the VSYS ref voltage for reading by ADC if already hasn't been.
	if(!_vSysRef_setup)
	{
		adc_gpio_init(VSYS_REF_GPIO);

		_vSysRef_setup = true;
	}
}

double bosch_map_0261230119::readPsi()
{
	return __readKpa() / 0.1450377377;
}

double bosch_map_0261230119::readKpa()
{
	return __readKpa();
}

double bosch_map_0261230119::__readKpa()
{
	// Note: The ENOB (effective number of bits) on the pi pico is 8.7, so just taking the most significant 9 bits
	//       probably makes more sense at this stage.

	adc_select_input(_adcInput);

	uint16_t boschMapOutAdc = adc_read();

	printf("mapOutAdc: %u\n", boschMapOutAdc);

	double vsys = __readVSys();

	printf("vsys: %f\n", vsys);

	// The actual bosch map sensor output, referenced to 5V.
	// The voltage scaling from 5V to 3.3V needs to be taken into account.
	double boschMapOut = boschMapOutAdc * 3.0 * _adcScale / 2.0;

	printf("mapOut: %f\n", boschMapOut);

	// This is the resultant equation from the Bosch 0261230119 map sensor data sheet.
	return ((boschMapOut / vsys) - _bosch_map_c0) / _bosch_map_c1;
}

double bosch_map_0261230119::__readVSys()
{
	adc_select_input(VSYS_REF_CHANNEL);

	uint16_t vsys_adc = adc_read();

	// Truncate the ADC read back to 9 bits.
	return (double) vsys_adc * 3.0 * _adcScale;
}