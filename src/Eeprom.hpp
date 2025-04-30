#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

#define EEPROM_MAGIC 0x55

/**
 * Wear levelled page definition for EEPROM.
 */
struct EepromPage
{
	/** Size of page in bytes. */
	uint8_t pageSize;

	/**
	 * The number of pages used for wear levelling. This is clamped to 15 bits.
	 * @note The total amout of memory used to store a page will be wearCount * (pageSize + 2)
	 */
	uint16_t wearCount;
};

/**
 * Base of drivers for EEPROM chips.
 * @note All data is stored using little endian byte order.
 */
class Eeprom
{
	public:

		virtual ~Eeprom();

		/**
		 * @param size Size of EEPROM in bytes.
		 * @param pages Array of wear levelled pages. The index into this array needs to be used for future page accesses.
		 * @param pageCount Number of entries in the pages array. Clamped to 8 bit number.
		 * @note Wear levelled pages are always stored at the beginning of the EEPROM.
		 * @note Memory calculations must take into account that the first bytes are reserved for the magic number and
		 *       page information: (sizeof EepromPage) * pageCount + 2.
		 */
		Eeprom(unsigned size, EepromPage* pages, uint8_t pageCount);

		/**
		 * Clear the Eeprom to the given value for the given region.
		 * @param value Value to set each byte in region to.
		 * @param start Start address or region.
		 * @param count Number of bytes to set.
		 */
		virtual void clear(uint8_t value, unsigned start, unsigned count) = 0;

		/**
		 * Write wear balanced page to Eeprom.
		 * @param pageId Identifier of page to write. Only the first ??? bits of this are used.
		 * @param page Point to page data to write. Must be page length in size.
		 */
		void writePage(unsigned pageId, uint32_t* page);

	protected:

		/**
		 * Write the given bytes to the Eeprom.
		 * @param startAddr Address to start writing at.
		 * @param values Pointer to byte values to write.
		 * @param count Number of bytes to write.
		 */
		virtual void _writeBytes(uint32_t startAddr, uint8_t* values, unsigned count) = 0;

		/**
		 * Read bytes from the Eeprom.
		 * @param startAddr Address to start reading from.
		 * @param values Pointer to byte array to read into.
		 * @param count Number of bytes to read.
		 */
		virtual void _readBytes(uint32_t startAddr, uint8_t* values, unsigned count) = 0;

	private:

		/** Size of EEPROM in bytes. */
		unsigned _eepromSize;

		/** Wear levelled pages. */
		EepromPage* _pages;

		/** Number of wear levelled pages. */
		unsigned _pageCount;
};

#endif