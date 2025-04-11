#include <stdio.h>

#include "hardware/adc.h"

#include "bosch_map_0261230119.hpp"

bool bosch_map_0261230119::_vSysRef_setup = false;

// If VRef is sourced from the power supply, the Pico has a ~30mV offset from the nominal 3.3V power supply.
double bosch_map_0261230119::_adcScale = 3.27 / (double)(1 << 12);

bosch_map_0261230119::~bosch_map_0261230119()
{
	delete[] _rawPressureVals;
	delete[] _rawVSysVals;
}

bosch_map_0261230119::bosch_map_0261230119(unsigned adcInput, unsigned avgCount) : _adcInput(adcInput), _avgCount(avgCount)
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

	_rawPressureVals = new uint16_t[avgCount];
	_rawVSysVals = new uint16_t[avgCount];

	for(int index = 0; index < avgCount; index++)
	{
		_rawPressureVals[index] = 0;
		_rawVSysVals[index] = 0;
	}
}

void bosch_map_0261230119::latch()
{
	adc_select_input(_adcInput);
	_rawPressureVals[_curRawValPosn] = adc_read();

	adc_select_input(VSYS_REF_CHANNEL);
	_rawVSysVals[_curRawValPosn] = adc_read();

	_curRawValPosn++;
	if(_curRawValPosn >= _avgCount) _curRawValPosn = 0;
}

void bosch_map_0261230119::__calcRawAvgVals()
{
	unsigned divisor = 0;

	_lastRawPressureAvg = 0;
	_lastRawVSysAvg = 0;

	uint16_t rawVal;

	for(int index = 0; index < _avgCount; index++)
	{
		rawVal = _rawPressureVals[index];

		if(rawVal != 0)
		{
			_lastRawPressureAvg += rawVal;
			divisor++;
		}

		_lastRawVSysAvg += _rawVSysVals[index];
	}

	if(divisor > 0)
	{
		_lastRawPressureAvg /= divisor;
		_lastRawVSysAvg /= divisor;
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
	__calcRawAvgVals();

	printf("mapOutAdc: %u\n", _lastRawPressureAvg);

	double vsys = __readVSys();

	printf("vsys: %f\n", vsys);

	// The actual bosch map sensor output, referenced to 5V.
	// The voltage scaling from 5V to 3.3V needs to be taken into account.
	double boschMapOut = _lastRawPressureAvg * 3.0 * _adcScale / 2.0;

	printf("mapOut: %f\n", boschMapOut);

	// This is the resultant equation from the Bosch 0261230119 map sensor data sheet.
	return ((boschMapOut / vsys) - _bosch_map_c0) / _bosch_map_c1;
}

double bosch_map_0261230119::__readVSys()
{
	return (double) _lastRawVSysAvg * 3.0 * _adcScale;
}