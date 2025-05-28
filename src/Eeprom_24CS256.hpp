#ifndef EEPROM_24CS256_H
#define EEPROM_24CS256_H

#include <stdint.h>

#include "hardware/i2c.h"

#include "Eeprom.hpp"

/**
 * The timeout, in micro seconds, to apply on per byte transfered.
 * This is currently based on i2c default speed mode of 100 kbit/s.
 */
#define PER_BYTE_TIMEOUT_US 100

/**
 * The amount of overhead in terms of the number of bytes transferred on a typical read/write before any actual data is
 * tranferred.
 */
#define READ_WRITE_TIMEOUT_OVERHEAD 4

/**
 * Driver for 24CS256 EEPROM chip.
 */
class Eeprom_24CS256 : public Eeprom
{
	public:

		virtual ~Eeprom_24CS256();

		/**
		 * @param i2cBus i2c Bus to communicate to EEPROM on. Assumes it has already been initialised.
		 * @param i2cAddr 3 bit address of the EEPROM chip.
		 * @param pages Array of wear levelled pages. The index into this array needs to be used for future page accesses.
		 * @param pageCount Number of entries in the pages array. Clamped to 8 bit number.
		 */
		Eeprom_24CS256(i2c_inst_t* i2cBus, uint8_t i2cAddr, EepromPage* pages, uint8_t pageCount);

	protected:

		// Impl.
		void _clear(uint8_t value, unsigned start, unsigned count);

		// Impl.
		bool _writeBytes(uint32_t startAddr, uint8_t* values, unsigned count);

		// Impl.
		bool _readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count);

	private:

		/** i2c bus (instance) that EEPROM is attached to. */
		i2c_inst_t* _i2cBus;

		/**
		 * Address of the EEPROM chip, clamped to 3 bits (which is the maximum this chip supports).
		 */
		uint8_t _i2cAddr;

		/**
		 * Calculate the timeout required for a number of bytes transferred.
		 * @param numBytesTransf Number of bytes that will be transferred to/from chip.
		 */
		unsigned __calcTimeout(unsigned numBytesTransf);
};

#endif