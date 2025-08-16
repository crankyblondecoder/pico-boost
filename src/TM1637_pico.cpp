#include "TM1637_pico.hpp"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "pico/platform.h"

TM1637Display::TM1637Display(uint8_t clk, uint8_t dio) : _clk(clk), _dio(dio)
{
	// Aim for 2 microsecond clock cycle duration.
	_cpuCyclesPerQuarterClock = clock_get_hz(clk_sys) / 2000000;
	_cpuCyclesPerHalfClock = _cpuCyclesPerQuarterClock * (uint32_t) 2;
	_cpuCyclesPerClock = _cpuCyclesPerHalfClock * (uint32_t) 2;

	gpio_init(_clk);
	gpio_set_dir(_clk, GPIO_OUT);
	gpio_init(_dio);
	gpio_set_dir(_dio, GPIO_OUT);

	// Put the chip into the stop state. This should be the state it stays in unless data is being transmitted to it.
	gpio_put(_clk, 1);
	gpio_put(_dio, 1);

	// Wait a full clock to give it time to react.
	busy_wait_at_least_cycles(_cpuCyclesPerClock);

	clear();
}

void TM1637Display::__start()
{
	// "The start condition of data input is that when CLK is high, DIO changes from high to low.

	// Assume the GPIO's are already in the stop state.

	// High to low indicates start.
	gpio_put(_dio, 0);

	// Wait a full clock. This comes from the start of the timing diagrams from the datasheet.
	busy_wait_at_least_cycles(_cpuCyclesPerClock);

	// Set clock ready for data to be written. This is required by write function.
	gpio_put(_clk, 0);
}

void TM1637Display::__stop()
{
	// "The end condition is that when CLK is high, DIO changes from low to high."

	// Assume clk and dio are both low entering into this function.

	// Wait half a clock because a stop always follows the falling edge of the end of the ACK.
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);

	gpio_put(_clk, 1);
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);
	gpio_put(_dio, 1);

	// Wait another half clock so that stop has holding period.
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);
}

uint8_t TM1637Display::__writeByte(uint8_t data)
{
	// "When inputting data, when CLK is high level, the signal on DIO must remain unchanged;
	// only when the clock signal on CLK is low level, the signal on DIO can Change."

	// In the data sheet, Data Creation Time (tSETUP) is 100ns. This is the minimum time required, after the data line is
	// set, to wait before the clock rising edge.

	// In the data sheet, Data Retention Time (tHOLD) is 100ns. This is the minimum time required, after the clock rising edge,
	// that the signal on the data line must be kept steady.

	// Assume clock low has been set prior to entry of this function.

	uint8_t ack = 0;

	for(uint8_t clockCount = 0; clockCount < 8; clockCount++)
	{
		// Write data on clock low.

		// Wait a quarter clock after previous falling edge before setting dio. This should also give more than
		// enough setup time prior to clk going high.
		busy_wait_at_least_cycles(_cpuCyclesPerQuarterClock);
		gpio_put(_dio, (data & 0x01) ? 1 : 0);
		data >>= 1;

		// Allow another quarter clock to satisfy tSetup.
		busy_wait_at_least_cycles(_cpuCyclesPerClock);

		// Latch data on rising edge.
		gpio_put(_clk, 1);

		// Wait half clock to satisfy tHOLD.
		busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);

		// Create clock falling edge ready for next bit transfer.
		gpio_put(_clk, 0);
	}

	// At this point the clock should be at the end of the falling edge.

	// "The data transmission of TM1637 has the response signal ACK. When the transmitted data is correct, at the falling
	// edge of the eighth clock, the chip will generate a response signal ACK to pull the DIO pin low, and release the DIO
	// after the end of the ninth clock."

	// Get dio ready to read ACK.
	gpio_set_dir(_dio, GPIO_IN);
	// The dio pin is reported to be open drain so needs a pull up resistor to be read accurately.
	gpio_pull_up(_dio);

	// Timing diagram suggests ACK at clock low should last a full clock.
	// Sample in the middle of that period.
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);
	ack = gpio_get(_dio);
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);

	// Rising edge of 9th clock.
	gpio_put(_clk, 1);
	// Half clock of "top" of waveform.
	busy_wait_at_least_cycles(_cpuCyclesPerHalfClock);
	// Falling edge. ACK period should end.
	gpio_put(_clk, 0);

	// Set dio back to output and pull low as default. This makes it ready for a stop if necessary.
	gpio_set_dir(_dio, GPIO_OUT);
	gpio_put(_dio, 0);

	return ack;
}

void TM1637Display::setBrightness(uint8_t brightness)
{
	// Max brightness is 0x07.
	_brightness = brightness & 0x07;
}

void TM1637Display::clear()
{
	uint8_t data[] = {0, 0, 0, 0};
	show(data);
}

void TM1637Display::show(uint8_t data[4])
{
	__start();
	__writeByte(TM1637_CMD1);
	__stop();

	__start();
	__writeByte(TM1637_CMD2);

	for(int index = 0; index < 4; ++index)
	{
		__writeByte(data[index]);
	}

	__stop();

	__start();
	__writeByte(TM1637_CMD3 + _brightness);
	__stop();
}

void TM1637Display::show(uint8_t position, uint8_t data)
{
	if(position > 3)
	{
		return;
	}

	__start();
	__writeByte(TM1637_CMD1);
	__stop();

	__start();
	__writeByte(TM1637_CMD2 + position);
	__writeByte(data);
	__stop();

	__start();
	__writeByte(TM1637_CMD3 + _brightness);
	__stop();
}

uint8_t TM1637Display::encodeDigit(unsigned digit)
{
	if(digit > 9)
	{
		// Handle out-of-range digits.
		return 0;
	}

	return _digitToSegment[digit];
}

void TM1637Display::encodeNumber(unsigned number, unsigned numDigits, unsigned startPosn, uint8_t data[4])
{
	int curEncodePosn = startPosn > 3 ? 3 : startPosn;
	unsigned digitCount = numDigits > 4 ? 4 : numDigits;

	while(curEncodePosn >= 0 && digitCount > 0)
	{
		data[curEncodePosn] = encodeDigit(number % 10);

		number /= 10;
		curEncodePosn--;
		digitCount--;
	}
}

uint8_t TM1637Display::encodeAlpha(char character)
{
	uint8_t retVal = 0;

	switch(character)
	{
		case 'a':
		case 'A':
			retVal = 0x77;
			break;
//*
		case 'b':
		case 'B':
			retVal = 0x7C;
			break;
//*
		case 'c':
		case 'C':
			retVal = 0x39;
			break;
//*
		case 'd':
		case 'D':
			retVal = 0x5E;
			break;
//*
		case 'e':
		case 'E':
			retVal = 0x79;
			break;

		case 'f':
		case 'F':
			retVal = 0x71;
			break;
//*
		case 'h':
		case 'H':
			retVal = 0x76;
			break;
//*
		case 'j':
		case 'J':
			retVal = 0x1E;
			break;
//*
		case 'l':
		case 'L':
			retVal = 0x38;
			break;
//*
		case 'o':
		case 'O':
			retVal = 0x5C;
			break;
//*
		case 'p':
		case 'P':
			retVal = 0x73;
			break;
//*
		case 'q':
		case 'Q':
			retVal = 0x67;
			break;
//*
		case 'r':
		case 'R':
			retVal = 0x50;
			break;
//*
		case 't':
		case 'T':
			retVal = 0b01111000;
			break;
//*
		case 'u':
		case 'U':
			retVal = 0x3E;
			break;

		case 'y':
		case 'Y':
			retVal = 0x6E;
			break;

		case '-':
			retVal = 0x40;
			break;
	}

	return retVal;
}

const uint8_t TM1637Display::_digitToSegment[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};