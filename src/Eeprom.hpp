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
 * This keeps track of the current page instance as well as other data required to process them.
 * A page instance is a particular wear levelled page in the page wear levelling region (The region where multiple indexed
 * copies of the same page are stored).
 */
struct EepromPageInstance
{
	/**
	 * Wear index associated with page. This starts at 1 for valid indexes and increases as pages are written on a
	 * per page definition basis.
	 * A value of 0 indicates that there are no pages present for the associated page description.
	 */
	uint16_t wearIndex;

	/**
	 * The physical page index the page instance is stored in.
	 * This will be between 0 and wearCount - 1 (from the page definition).
	 */
	uint16_t pageIndex;

	/**
	 * The byte address of the start of the wear levelled region for the page.
	 */
	uint32_t regionStartAddress;
};

/**
 * Base of drivers for EEPROM chips.
 * @note All data is stored using little endian byte order and the Pico is assumed to be running in little endian mode.
 * @note This class does _not_ do any bounds checks.
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
		 * Write the given bytes to the Eeprom.
		 * @param startAddr Address to start writing at.
		 * @param values Pointer to byte values to write.
		 * @param count Number of bytes to write.
		 * @returns True if write was successful.
		 */
		bool writeBytes(uint32_t startAddr, uint8_t* values, unsigned count);

		/**
		 * Read bytes from the Eeprom.
		 * @param startAddr Address to start reading from.
		 * @param buffer Pointer to byte array to read into.
		 * @param count Number of bytes to read.
		 * @returns True if read was successful.
		 */
		bool readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count);

		/**
		 * Read wear balanced page from Eeprom.
		 * @param pageId Identifier of page to read. This should match the index of the pages defined during construction.
		 * @param pageBuffer Pointer to buffer to read page data into. Must be page length in size.
		 * @returns True if read was successful. False if no page available or error.
		 */
		bool readPage(uint8_t pageId, uint8_t* pageBuffer);

		/**
		 * Write wear balanced page to Eeprom.
		 * @param pageId Identifier of page to write. This should match the index of the pages defined during construction.
		 * @param pageData Pointer to page data to write. Must be page length in size.
		 * @returns True if write was successful.
		 */
		bool writePage(uint8_t pageId, uint8_t* pageData);

		/**
		 * Clear the Eeprom to the given value for the given region.
		 * @param value Value to set each byte in region to.
		 * @param start Start address or region.
		 * @param count Number of bytes to set.
		 */
		void clear(uint8_t value, unsigned start, unsigned count);

		/**
		 * Get the start address of the non-paged region.
		 * This is always after the paged region. ie At a higher address.
		 */
		uint32_t getNonPageRegionStartAddress();

		/**
		 * Independantly verify this EEPROM's meta data.
		 * @param pages Array of expected wear levelled pages.
		 * @param pageCount Number of entries in the pages array.
		 * @returns True if verified.
		 */
		bool verifyMetaData(EepromPage* pages, uint8_t pageCount);

	protected:

		/**
		 * Initialise data structures.
		 * @note This MUST be called by the implementing class once its construction is complete.
		 */
		void _init();

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
		 * @returns True if write was successful.
		 */
		virtual bool _writeBytes(uint32_t startAddr, uint8_t* values, unsigned count) = 0;

		/**
		 * Read bytes from the Eeprom.
		 * @param startAddr Address to start reading from.
		 * @param buffer Pointer to byte array to read into.
		 * @param count Number of bytes to read.
		 * @returns True if read was successful.
		 */
		virtual bool _readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count) = 0;

	private:

		/** Size of EEPROM in bytes. */
		unsigned _eepromSize;

		/** Wear levelled page descriptors. */
		EepromPage* _pages;

		/** Number of wear levelled pages. */
		uint8_t _pageCount;

		/** Current page instances. The index matches the indexes of the page definitions. */
		EepromPageInstance* _pageInstances;

		/** The start address of the non-page region. The region after the wear levelled pages. */
		uint32_t _nonPageRegionStartAddress;
};

#endif