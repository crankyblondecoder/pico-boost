#include "hardware/adc.h"

#include "bosch_map_0261230119.hpp"

bosch_map_0261230119::bosch_map_0261230119(unsigned adcInput) : _adcInput(adcInput)
{
	unsigned GPIO_PIN = adcInput + 26;

	// This should make the GPIO high-impedance.
    adc_gpio_init(GPIO_PIN);
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
	adc_select_input(0);
	uint16_t bosch_map_out_adc = adc_read();

	// This is the resultant equation from the Bosch 0261230119 map sensor data sheet.
	return (((double)bosch_map_out_adc / 4096.0) - _bosch_map_c0) / _bosch_map_c1;
}