#include "BoostControl.hpp"

extern bool debug;

BoostControl::~BoostControl()
{
	delete _pwmControl;
	delete _vsysRefAdc;
	delete _mapSensor;
}

BoostControl::BoostControl()
{
	populateDefaultParameters(&_curParams);

	_pwmControl = new PicoPwm(CONTROL_SOLENOID_CHAN_A_GPIO, CONTROL_SOLENOID_CHAN_A_GPIO + 1, CONTROL_SOLENOID_FREQ, 0, 0, true,
			CONTROL_SOLENOID_DISABLE_GATE_STATE);

	// Create ADC reader to read VSys. The Pico divides this voltage by 3.
	_vsysRefAdc = new PicoAdcReader(3, 10, 3.0, 3.0);

	// Start map sensor on ADC Channel 0. This should map to GP26.
	// Voltage divider scale is calculated from (R1 + R2) / R2.
	_mapSensor = new BoschMap_0261230119(0, 3.0, (2.2 + 3.2) / 3.2, _vsysRefAdc);

	_nextBoostLatchTime = get_absolute_time();
	_nextBoostReadTime = _nextBoostLatchTime;
	_lastSolenoidProcTime = _nextBoostLatchTime;

	_initialised = true;
}

bool BoostControl::ready()
{
	return _initialised;
}

void BoostControl::getParameters(BoostControlParameters* params)
{
	params -> maxKpaScaled = _curParams.maxKpaScaled;

	params -> deEnergiseKpaScaled = _curParams.deEnergiseKpaScaled;

	params -> pidActiveKpaScaled = _curParams.pidActiveKpaScaled;

	params -> pidPropConstScaled = _curParams.pidPropConstScaled;

	params -> pidIntegConstScaled = _curParams.pidIntegConstScaled;

	params -> pidDerivConstScaled = _curParams.pidDerivConstScaled;

	params -> maxDuty = _curParams.maxDuty;

	params -> zeroPointDuty = _curParams.zeroPointDuty;
}

void BoostControl::setParameters(BoostControlParameters* params)
{
	__setMaxKpaScaled(params -> maxKpaScaled);

	__setDeEnergiseKpaScaled(params -> deEnergiseKpaScaled);

	__setPidActiveKpaScaled(params -> pidActiveKpaScaled);

	__setPidPropConstScaled(params -> pidPropConstScaled);

	__setPidIntegConstScaled(params -> pidIntegConstScaled);

	__setPidDerivConstScaled(params -> pidDerivConstScaled);

	__setMaxDutyScaled(params -> maxDuty);

	__setZeroPointDutyScaled(params -> zeroPointDuty);
}

void BoostControl::populateDefaultParameters(BoostControlParameters* params)
{
	// These are just abitrary and don't have any significance other than that they might be reasonable for 15 psi of boost.

	params -> maxKpaScaled = 100000;

	params -> deEnergiseKpaScaled = 50000;

	params -> pidActiveKpaScaled = 75000;

	params -> pidPropConstScaled = 6000;

	params -> pidIntegConstScaled = 1000;

	params -> pidDerivConstScaled = 500;

	params -> maxDuty = 950;

	params -> zeroPointDuty = 500;
}

void BoostControl::poll()
{
	absolute_time_t curTime = get_absolute_time();

	if(debug || curTime >= _nextBoostLatchTime)
	{
		// Latch data at approximately 1000hz. This is a higher frequency to allow for averaging to be effective.
		_nextBoostLatchTime = delayed_by_ms(_nextBoostLatchTime, 1);

		_mapSensor -> latch();
	}

	if(debug || curTime >= _nextBoostReadTime)
	{
		// Process map sensor and control solenoid at approximately 100hz
		_nextBoostReadTime = delayed_by_ms(_nextBoostReadTime, 10);

		_mapKpaScaled = _mapSensor -> readKpa() * 1000.0;

		double boost_atm_scaled = (double)_mapKpaScaled - STD_ATM_PRESSURE;

		_energised = boost_atm_scaled >= _curParams.deEnergiseKpaScaled;

		__processControlSolenoid();
	}
}

bool BoostControl::isEnergised()
{
	return _energised;
}

bool BoostControl::isMaxBoostReached()
{
	return getKpaScaled() >= (int)_curParams.maxKpaScaled;
}

int BoostControl::getKpaScaled()
{
	return _mapKpaScaled - STD_ATM_PRESSURE;
}

int BoostControl::getPsiScaled()
{
	return (getKpaScaled() / (float)1000.0) * KPA_TO_PSI * 10;
}

unsigned BoostControl::getMaxKpaScaled()
{
	return _curParams.maxKpaScaled;
}

void BoostControl::__setMaxKpaScaled(unsigned maxKpaScaled)
{
	_curParams.maxKpaScaled = maxKpaScaled;
}

void BoostControl::alterMaxKpaScaled(int maxBoostKpaScaledDelta)
{
	int newVal = (int)_curParams.maxKpaScaled + maxBoostKpaScaledDelta;

	if(newVal > 0) _curParams.maxKpaScaled = newVal; else _curParams.maxKpaScaled = 0;
}

unsigned BoostControl::getDeEnergiseKpaScaled()
{
	return _curParams.deEnergiseKpaScaled;
}

void BoostControl::__setDeEnergiseKpaScaled(unsigned kpaScaled)
{
	_curParams.deEnergiseKpaScaled = kpaScaled;
}

void BoostControl::alterDeEnergiseKpaScaled(int deEnergiseKpaScaledDelta)
{
	int newVal = (int)_curParams.deEnergiseKpaScaled + deEnergiseKpaScaledDelta;

	if(newVal > 0) _curParams.deEnergiseKpaScaled = newVal; else _curParams.deEnergiseKpaScaled = 0;
}

unsigned BoostControl::getPidActiveKpaScaled()
{
	return _curParams.pidActiveKpaScaled;
}

void BoostControl::__setPidActiveKpaScaled(unsigned kpaScaled)
{
	_curParams.pidActiveKpaScaled = kpaScaled;
}

void BoostControl::alterPidActiveKpaScaled(int delta)
{
	int newVal = (int)_curParams.pidActiveKpaScaled + delta;

	if(newVal > 0) _curParams.pidActiveKpaScaled = newVal; else _curParams.pidActiveKpaScaled = 0;
}

unsigned BoostControl::getPidPropConstScaled()
{
	return _curParams.pidPropConstScaled;
}

void BoostControl::__setPidPropConstScaled(unsigned pidPropConstScaled)
{
	_curParams.pidPropConstScaled = pidPropConstScaled;
}

void BoostControl::alterPidPropConstScaled(int pidPropConstScaledDelta)
{
	int newVal = (int)_curParams.pidPropConstScaled + pidPropConstScaledDelta;

	if(newVal > 0) _curParams.pidPropConstScaled = newVal; else _curParams.pidPropConstScaled = 0;
}

unsigned BoostControl::getPidIntegConstScaled()
{
	return _curParams.pidIntegConstScaled;
}

void BoostControl::__setPidIntegConstScaled(unsigned pidIntegConstScaled)
{
	_curParams.pidIntegConstScaled = pidIntegConstScaled;
}

void BoostControl::alterPidIntegConstScaled(int pidIntegConstScaledDelta)
{
	int newVal = (int)_curParams.pidIntegConstScaled + pidIntegConstScaledDelta;

	if(newVal > 0) _curParams.pidIntegConstScaled = newVal; else _curParams.pidIntegConstScaled = 0;
}

unsigned BoostControl::getPidDerivConstScaled()
{
	return _curParams.pidDerivConstScaled;
}

void BoostControl::__setPidDerivConstScaled(unsigned pidDerivConstScaled)
{
	_curParams.pidDerivConstScaled = pidDerivConstScaled;
}

void BoostControl::alterPidDerivConstScaled(int pidDerivConstScaledDelta)
{
	int newVal = (int)_curParams.pidDerivConstScaled + pidDerivConstScaledDelta;

	if(newVal > 0) _curParams.pidDerivConstScaled = newVal; else _curParams.pidDerivConstScaled = 0;
}

unsigned BoostControl::getMaxDutyScaled()
{
	return _curParams.maxDuty;
}

void BoostControl::__setMaxDutyScaled(unsigned maxDuty)
{
	_curParams.maxDuty = maxDuty;

	if(_curParams.maxDuty > 999) _curParams.maxDuty = 999;
}

void BoostControl::alterMaxDutyScaled(int maxDutyDelta)
{
	int newVal = (int)_curParams.maxDuty + maxDutyDelta;

	if(newVal > 0) _curParams.maxDuty = newVal; else _curParams.maxDuty = 0;

	if(_curParams.maxDuty > 999) _curParams.maxDuty = 999;
}

unsigned BoostControl::getZeroPointDutyScaled()
{
	return _curParams.zeroPointDuty;
}

void BoostControl::__setZeroPointDutyScaled(unsigned zeroPointDuty)
{
	_curParams.zeroPointDuty = zeroPointDuty;

	if(_curParams.zeroPointDuty > 999) _curParams.zeroPointDuty = 999;
}

void BoostControl::alterZeroPointDutyScaled(int dutyDelta)
{
	int newVal = (int)_curParams.zeroPointDuty + dutyDelta;

	if(newVal > 0) _curParams.zeroPointDuty = newVal; else _curParams.zeroPointDuty = 0;

	if(_curParams.zeroPointDuty > 999) _curParams.zeroPointDuty = 999;
}

unsigned BoostControl::getCurrentDutyScaled()
{
	if(_energised) return _pwmControl -> getDutyA() * 10.0;

	return 0;
}

void BoostControl::__setSolenoidDuty(float duty)
{
	_pwmControl -> setDuty(duty, -1);
}

void BoostControl::__enableSolenoid()
{
	_pwmControl -> enable();
}

void BoostControl::__disableSolenoid()
{
	_pwmControl -> disable(CONTROL_SOLENOID_DISABLE_GATE_STATE);
}

void BoostControl::__processControlSolenoid()
{
	if(!_testMode)
	{
		int curBoostScaled = _mapKpaScaled - STD_ATM_PRESSURE;

		// Apply hysterisis to enable/disable about de-energise point.
		if(_energised && curBoostScaled < ((int)_curParams.deEnergiseKpaScaled - CONTROL_DE_ENERGISE_HYSTERESIS))
		{
			_energised = false;
			__disableSolenoid();
		}
		else if(!_energised && curBoostScaled > (int)_curParams.deEnergiseKpaScaled)
		{
			_energised = true;
			__enableSolenoid();
		}

		if(_energised)
		{
			if(curBoostScaled < (int)_curParams.pidActiveKpaScaled)
			{
				// Pin at max duty to get maximum boost.
				__setSolenoidDuty(_curParams.maxDuty);

				_pidActive = false;
			}
			else
			{
				absolute_time_t cur_proc_time = get_absolute_time();

				if(!_pidActive)
				{
					// Setup initial PID vars.
					_pidPrevError = 0;
					_pidInteg = 0;
					_lastPidProcTime = cur_proc_time;

					_pidActive = true;
				}

				float error = (curBoostScaled - (int)_curParams.maxKpaScaled) / 1000.0;

				float deltaTime = absolute_time_diff_us(_lastPidProcTime, cur_proc_time) / 1000.0;

				// Calc proportional and deriviative terms.
				float controlVar = error * (float)_curParams.pidPropConstScaled + (error - _pidPrevError) *
					(float)_curParams.pidDerivConstScaled;

				// Use an approximation to a time limited integration term.
				// This removes a proportion of the average from the term and adds in the value associated with the current
				// delta time.
				_pidInteg -= deltaTime * _pidInteg / CONTROL_PID_INTEG_SUM_TIME;
				_pidInteg += error * deltaTime;

				controlVar += _pidInteg * (float)_curParams.pidIntegConstScaled;

				// Map control var to duty cycle and set duty cycle.
				// Use one to one correspondence between control var and duty cycle with zero point adjustment so that
				// a control var of zero should match the required boost output.

				float duty = controlVar + (float)_curParams.zeroPointDuty / 10.0;

				float maxDuty = (float)_curParams.maxDuty / 10.0;

				if(duty > maxDuty) duty = maxDuty;
				if(duty < 0.0) duty = 0.0;

				__setSolenoidDuty(duty);

				_lastPidProcTime = cur_proc_time;

				_pidPrevError = error;
			}
		}
		else
		{
			_pidActive = false;
		}
	}
}

void BoostControl::testSolenoid()
{
	_testMode = true;

	__enableSolenoid();

	for(float duty = 0; duty < 100; duty += 1)
	{
		__setSolenoidDuty(duty);
		sleep_ms(100);
	}

	/*
	set_solenoid_duty(5);
	sleep_ms(60000);
	set_solenoid_duty(0);
	sleep_ms(1000);
	*/

	__disableSolenoid();

	_testMode = false;
}

double BoostControl::mapReadSupplyVoltage()
{
	return _mapSensor -> readSupplyVoltage();
}

double BoostControl::mapReadSensorVoltage()
{
	return _mapSensor -> readSensorVoltage();
}
