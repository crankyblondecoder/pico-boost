#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

/** Magic byte to indicate EEPROM has been formated by this class. */
#define EEPROM_MAGIC 0x55

/** Address of wear levelled page count. */
#define EEPROM_PAGE_COUNT_ADDR 0x01

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
 * @note All data is stored using little endian byte order and the Pico is assumed to be running in little endian mode.
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
		 * Read wear balanced page from Eeprom.
		 * @param pageId Identifier of page to read. This should match the index of the pages defined during construction.
		 * @param page Pointer to buffer to read page data into. Must be page length in size.
		 */
		void readPage(uint8_t pageId, uint8_t* page);

		/**
		 * Write wear balanced page to Eeprom.
		 * @param pageId Identifier of page to write. This should match the index of the pages defined during construction.
		 * @param page Pointer to page data to write. Must be page length in size.
		 */
		void writePage(uint8_t pageId, uint8_t* page);

		/**
		 * Clear the Eeprom to the given value for the given region.
		 * @param value Value to set each byte in region to.
		 * @param start Start address or region.
		 * @param count Number of bytes to set.
		 */
		void clear(uint8_t value, unsigned start, unsigned count);

	protected:

		/**
		 * Clear the Eeprom to the given value for the given region.
		 * @param value Value to set each byte in region to.
		 * @param start Start address or region.
		 * @param count Number of bytes to set.
		 */
		virtual void _clear(uint8_t value, unsigned start, unsigned count) = 0;

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

		/** Wear levelled page descriptors. */
		EepromPage* _pages;

		/** Number of wear levelled pages. */
		uint8_t _pageCount;

		/**
		 * Current wear indexes.
		 * Array indexes match page indexes.
		 * A value of 0 is the null index. ie Indicates no pages currently present.
		 * @note A wear index of 0xFFFF physically present indicates a blank region and isn't a valid wear index. It will
		 *       never be present in this array.
		 */
		uint16_t* _curWearIndexes;

		/**
		 * Current wear pages.
		 * Array indexes match page indexes.
		 * The value is the page index that holds the current page data. It is clamped to being less than the wear count.
		 */
		uint16_t* _curWearPage;
};

#endif