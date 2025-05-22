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
	// NOTE: To reference the device as EEPROM (there are other modes), bits 7-4 must be 1010

	// Buffer to allow up to 64 byte page write. First extra two bytes are for the word address.
	// The EEPROM addressing requires the high order address bits first so an address has to be assembled byte by byte.
	uint8_t buffer[66];

	uint32_t writeAddr = startAddr;
	uint32_t nextAddr = startAddr;

	unsigned numToWriteInPage;
	unsigned totalNumWritten = 0;

	int response = 1;

	while(response > 0 && totalNumWritten < count)
	{
		// Doesn't include address bytes.
		numToWriteInPage = 0;

		// Assemble the buffer to write.
		// Lowest order 6 bits is the address within a page.
		do {

			buffer[numToWriteInPage + 2] = values[totalNumWritten];

			numToWriteInPage++;
			totalNumWritten++;
			nextAddr++;

		} while(totalNumWritten < count && (nextAddr & 0x3F));

		// Only 15bits of the address is kept.
		buffer[0] = (writeAddr & 0x7F00) >> 8;
		buffer[1] = writeAddr & 0xFF;

		// Write data to address.
		response = i2c_write_timeout_us(_i2cBus, 0x50 | (_i2cAddr & 0x7), buffer, numToWriteInPage + 2, false,
		__calcTimeout(2));

		writeAddr = nextAddr;

		// Allow for enough time for the chip to write to it's memory.
		// Twc (Write Cycle Time) is 5ms for this chip, as per the datasheet.
		sleep_ms(5);
	}
}

void Eeprom_24CS256::_readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count)
{
	// Write address to start reading from with no stop so that subsequent read can "restart" instead of doing a full
	// stop start cycle.

	// The EEPROM addressing requires the high order address bits first so an address has to be assembled byte by byte.
	uint8_t startAddrBytes[2];

	// Only 15bits of the address is kept.
	startAddrBytes[0] = (startAddr & 0x7F00) >> 8;
	startAddrBytes[1] = startAddr & 0xFF;

	// NOTE: To reference the device as EEPROM (there are other modes), bits 7-4 must be 1010.
	//       Only bits 0, 1 and 2 of the device address are usable.

	int response = i2c_write_timeout_us(_i2cBus, 0x50 | (_i2cAddr & 0x7), startAddrBytes, 2, true,
		__calcTimeout(2));

	// Either PICO_ERROR_GENERIC, PICO_ERROR_TIMEOUT or the number of bytes written is returned.
	if(response == 2)
	{
		// Now read data.
		i2c_read_timeout_us(_i2cBus, 0x50 | (_i2cAddr & 0x7), buffer, count, false, __calcTimeout(count));
	}
}

unsigned Eeprom_24CS256::__calcTimeout(unsigned numBytesTransf)
{
	return READ_WRITE_TIMEOUT_OVERHEAD + numBytesTransf * PER_BYTE_TIMEOUT_US;
}
