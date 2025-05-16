#include "hardware/gpio.h"

#include "PicoSwitch.hpp"

PicoSwitch::~PicoSwitch()
{
	gpio_deinit(_gpio);
}

PicoSwitch::PicoSwitch(unsigned gpio, PullUpDown pullUpDown, unsigned activeCountThreshold, unsigned sampleDuration) :
	_gpio(gpio), _pullUpDown(pullUpDown), _activeCountThreshold(activeCountThreshold), _sampleDuration(sampleDuration)
{
	gpio_init(gpio);
	gpio_set_dir(gpio, GPIO_IN);

	switch(pullUpDown)
	{
		case PULL_UP:

			gpio_pull_up(gpio);
			break;

		case PULL_DOWN:

			gpio_pull_down(gpio);
			break;
	}

	_lastSampleTime = get_absolute_time();
}

void PicoSwitch::poll()
{
	absolute_time_t curTime = get_absolute_time();

	if(absolute_time_diff_us(_lastSampleTime, curTime) >= _sampleDuration)
	{
		_lastSampleTime = curTime;

		bool switchState = gpio_get(_gpio);

		if(switchState)
		{
			if(_activeCount < _activeCountThreshold) _activeCount++;
		}
		else
		{
			if(_activeCount > 0) _activeCount--;
		}
	}

	// Look for current switch state changing.

	if(_activeCount == _activeCountThreshold || _activeCount == 0)
	{
		bool curState;

		if(_pullUpDown == PULL_UP)
		{
			curState = _activeCount == 0;
		}
		else
		{
			curState = _activeCount;
		}

		if(_currentState != curState)
		{
			if(curState)
			{
				_stateCycleCounter++;
			}
			else
			{
				_lastPressDuration = absolute_time_diff_us(_curStateTime, curTime) / 1000;
			}

			_currentState = curState;
			_curStateTime = curTime;
		}
	}
}

bool PicoSwitch::getSwitchState()
{
	return _currentState;
}

unsigned PicoSwitch::getSwitchStateDuration()
{
	return absolute_time_diff_us(_curStateTime, get_absolute_time()) / 1000;
}

unsigned PicoSwitch::getCurrentStateCycleIndex()
{
	return _stateCycleCounter;
}

unsigned PicoSwitch::getLastPressDuration()
{
	return _lastPressDuration;
}