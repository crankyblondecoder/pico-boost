#include <stdio.h>

#include "hardware/adc.h"

#include "BoschMap_0261230119.hpp"

extern bool debugMsgActive;

BoschMap_0261230119::~BoschMap_0261230119()
{
}

BoschMap_0261230119::BoschMap_0261230119(unsigned adcInput,  double vRef, double vScale, PicoAdcReader* vSysAdcReader) :
	_vSysAdcReader(vSysAdcReader)
{
	_picoAdcReader = new PicoAdcReader(adcInput, 10, vRef, vScale);
}

void BoschMap_0261230119::latch()
{
	_vSysAdcReader -> latch();
	_picoAdcReader -> latch();
}

double BoschMap_0261230119::readPsi()
{
	return __readKpa() / 0.1450377377;
}

double BoschMap_0261230119::readKpa()
{
	return __readKpa();
}

double BoschMap_0261230119::__readKpa()
{
	// The actual bosch map sensor output, referenced to 5V.
	double boschMapOut = _picoAdcReader -> read();

//printf("vmap: %f\n", boschMapOut);

	// Voltage supplied to MAP sensor.
	double vSys = _vSysAdcReader -> read();

//printf("vsys: %f\n", vSys);

	// This is the resultant equation from the Bosch 0261230119 map sensor data sheet.
	return ((boschMapOut / vSys) - _bosch_map_c0) / _bosch_map_c1;
}

double BoschMap_0261230119::readSupplyVoltage()
{
	return _vSysAdcReader -> read();
}

double BoschMap_0261230119::readSensorVoltage()
{
	return _picoAdcReader -> read();
}