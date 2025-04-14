#include "AdcReader.hpp"

AdcReader::~AdcReader()
{
	delete[] _rawVals;
}

AdcReader::AdcReader(unsigned adcInput, unsigned avgCount, double vRef, unsigned adcResolution, double scale) :
	_adcInput(adcInput), _avgCount(avgCount)
{
	_rawVals = new uint32_t[avgCount];

	for(int index = 0; index < avgCount; index++)
	{
		_rawVals[index] = 0;
	}

	_voltageScale = scale * vRef / (double)(1 << adcResolution);
}

void AdcReader::latch()
{
	_rawVals[_curRawValPosn] = _readFromAdc(_adcInput);

	_curRawValPosn++;

	if(_curRawValPosn >= _avgCount) _curRawValPosn = 0;
}

uint32_t AdcReader::readRaw()
{
	return __calcRawAvgVals();
}

double AdcReader::read()
{
	uint32_t rawAvgVal = __calcRawAvgVals();

	return (double)rawAvgVal * _voltageScale;
}

uint32_t AdcReader::__calcRawAvgVals()
{
	uint32_t sum = 0;

	for(unsigned index = 0; index < _avgCount; index++)
	{
		sum += _rawVals[index];
	}

	return sum / _avgCount;
}