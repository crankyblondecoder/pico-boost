#include "Eeprom_24CS256.hpp"

Eeprom_24CS256::Eeprom_24CS256(i2c_inst_t* i2cBus, uint8_t i2cAddr, EepromPage* pages, uint8_t pageCount)
	: Eeprom(32768, pages, pageCount), _i2cBus(i2cBus)
{
	_i2cAddr = i2cAddr & 0x07;

	Eeprom::_init();
}

Eeprom_24CS256::~Eeprom_24CS256()
{
}

void Eeprom_24CS256::_clear(uint8_t value, unsigned start, unsigned count)
{

}

void Eeprom_24CS256::_writeBytes(uint32_t startAddr, uint8_t* values, unsigned count)
{

}

void Eeprom_24CS256::_readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count)
{
	// Write address to start reading from with no stop so that subsequent read can "restart" instead of doing a full
	// stop start cycle.

	// This chip has a 16 bit word address that maxes out at 0x7FFF.
	uint16_t startAddr16 = startAddr;

	int response = i2c_write_timeout_us(_i2cBus, _i2cAddr, (uint8_t*)&startAddr16, 2, true,
		__calcTimeout(2));

	// Either PICO_ERROR_GENERIC, PICO_ERROR_TIMEOUT or the number of bytes written is returned.
	if(response == 2)
	{
		// Now read data.
		i2c_read_timeout_us(_i2cBus, _i2cAddr, buffer, count, false, __calcTimeout(count));
	}
}

unsigned Eeprom_24CS256::__calcTimeout(unsigned numBytesTransf)
{
	return READ_WRITE_TIMEOUT_OVERHEAD + numBytesTransf * PER_BYTE_TIMEOUT_US;
}
