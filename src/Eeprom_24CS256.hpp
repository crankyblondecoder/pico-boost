#ifndef EEPROM_24CS256_H
#define EEPROM_24CS256_H

#include <stdint.h>

#include "Eeprom.hpp"

/**
 * Driver for 24CS256 EEPROM chip.
 */
class Eeprom_24CS256 : public Eeprom
{
	public:

		virtual ~Eeprom_24CS256();

		/**
		 *
		 */
		Eeprom_24CS256();

	protected:

		// Impl
		void _writeBytes(uint32_t startAddr, uint8_t* values, unsigned count);

		// Impl
		void _readBytes(uint32_t startAddr, uint8_t* values, unsigned count);

	private:

		/** Page size used for wear leveling. Currently limited to 12 bits. */
		unsigned _pageSizeBits;
};

#endif