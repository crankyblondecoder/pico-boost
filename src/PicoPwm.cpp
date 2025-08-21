#include "math.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "PicoPwm.hpp"

PicoPwm::~PicoPwm()
{
}

PicoPwm::PicoPwm(int chanAGpio, int chanBGpio, float initFreq, float initDutyCycleA, float initDutyCycleB, bool phaseCorrect,
	bool initDisableState) : _chanAGpio(chanAGpio), _chanBGpio(chanBGpio)
{
	_curDutyA = 0.0;
	_curDutyB = 0.0;

	if(chanAGpio > -1) gpio_set_function(chanAGpio, GPIO_FUNC_PWM);
    if(chanBGpio > -1) gpio_set_function(chanBGpio, GPIO_FUNC_PWM);

	if(chanAGpio > -1)
	{
		_sliceNumber = pwm_gpio_to_slice_num(chanAGpio);
	}
	else if(chanBGpio > -1)
	{
		_sliceNumber = pwm_gpio_to_slice_num(chanBGpio);
	}
	else
	{
		_sliceNumber = -1;
	}

	_phaseCorrect = phaseCorrect ? 1 : 0;

	if(_sliceNumber > -1) pwm_set_phase_correct(_sliceNumber, phaseCorrect);

	__setFreq(initFreq);

	__setDuty(initDutyCycleA, initDutyCycleB);

	__disable(initDisableState);
}

bool PicoPwm::getEnabled()
{
	return _enabled;
}

void PicoPwm::enable()
{
	__enable();
}

void PicoPwm::__enable()
{
	if(_chanAGpio > -1) gpio_set_outover(_chanAGpio, GPIO_OVERRIDE_NORMAL);
	if(_chanBGpio > -1) gpio_set_outover(_chanBGpio, GPIO_OVERRIDE_NORMAL);

	if(_sliceNumber > -1) pwm_set_enabled(_sliceNumber, true);

	_enabled = true;
}

void PicoPwm::disable(bool setHigh)
{
	__disable(setHigh);
}

void PicoPwm::__disable(bool setHigh)
{
	if(_sliceNumber > -1) pwm_set_enabled(_sliceNumber, false);

	if(_chanAGpio > -1) gpio_set_outover(_chanAGpio, setHigh ? GPIO_OVERRIDE_HIGH : GPIO_OVERRIDE_LOW);
	if(_chanBGpio > -1) gpio_set_outover(_chanBGpio, setHigh ? GPIO_OVERRIDE_HIGH : GPIO_OVERRIDE_LOW);

	_enabled = false;
}

void PicoPwm::setFreq(float freq)
{
	__setFreq(freq);
}

void PicoPwm::__setFreq(float freq)
{
	if(_sliceNumber == -1) return;

	uint32_t f_sys = clock_get_hz(clk_sys);

	float f_ratio = (float) f_sys / freq;

	// Calculate maximum wrap (TOP) value that can be used to represent the required frequency.
	// Coerce that value into a 16 bit integer and reduce it enough to account for numerical error and that fact that
	// the channel level (CC) must be able to be wrap + 1 (TOP + 1) to achieve reliable 100% duty cycle.

	uint16_t counterWrap = (f_ratio / (_phaseCorrect + 1.0)) - 1;

	if(counterWrap > 0xFFF0) counterWrap = 0xFFF0;

	_counterWrap = counterWrap;
	pwm_set_wrap(_sliceNumber, counterWrap);

	// Full precision divider value.
	float divInt_divFrac = f_ratio / ((counterWrap + 1.0) * (_phaseCorrect + 1.0));

	// Split full prec divider value into integer and fraction parts.
	float intPart;
	float fracPart = modf(divInt_divFrac, &intPart);

	// Integer part of divider value.
	uint8_t divInt = intPart;

	// 4 bit (1/16) part of divider value.
	uint8_t divFrac = 16.0 * fracPart;

	// This together with the previously calculated wrap value should approximate the required frequency.
	pwm_set_clkdiv_int_frac4(_sliceNumber, divInt, divFrac);
}

float PicoPwm::getDutyA()
{
	return _curDutyA;
}

float PicoPwm::getDutyB()
{
	return _curDutyB;
}

void PicoPwm::setDuty(float dutyA, float dutyB)
{
	__setDuty(dutyA, dutyB);
}

void PicoPwm::__setDuty(float dutyA, float dutyB)
{
	if(_sliceNumber > -1)
	{
		// Note: For channel level (CC) to be at 100% duty cycle it must be set to wrap + 1 (TOP +1)

		if(dutyA >= 0)
		{
			pwm_set_chan_level(_sliceNumber, PWM_CHAN_A, (_counterWrap + 1) * (dutyA / 100.0));
			_curDutyA = dutyA;
		}

		if(dutyB >= 0)
		{
			pwm_set_chan_level(_sliceNumber, PWM_CHAN_B, (_counterWrap + 1) * (dutyB / 100.0));
			_curDutyB = dutyB;
		}
	}
}
