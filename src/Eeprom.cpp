#include "Eeprom.hpp"

Eeprom::~Eeprom()
{
	if(_pages) delete[] _pages;
}

Eeprom::Eeprom(unsigned size, EepromPage* pages, uint8_t pageCount)
{
	_eepromSize = size;
	_pages = 0;
	_pageCount = pageCount;

	// Total amount allocated to pages region. Includes page counters.
	unsigned totalAlloc;

	if(pageCount > 0)
	{
		// Copy the given pages.

		_pages = new EepromPage[pageCount];

		for(unsigned index = 0; index < pageCount; index++)
		{
			_pages[index].pageSize = pages[index].pageSize;
			_pages[index].wearCount = pages[index].wearCount & 0x7FFF;

			totalAlloc += _pages[index].wearCount * (_pages[index].pageSize + 2);
		}
	}

	// First bytes (header) of EEPROM are the magic number followed by the page count and then the pages info.
	// If any existing header doesn't match what is expected then the header is re-written and the pages region cleared.

	bool headerMatches = false;

	uint8_t magic;
	_readBytes(0, &magic, 1);

	if(magic == EEPROM_MAGIC)
	{
		uint8_t headerPageCount;
		_readBytes(1, &headerPageCount, 1);

		if(headerPageCount == pageCount)
		{
			uint8_t headerPageSize;
			uint16_t headerWearCount;

			for(unsigned index = 0; index < pageCount; index++)
			{
				_readBytes(1, &headerPageSize, 1);

				// TODO ...
			}
		}
	}

	if(headerMatches)
	{
		// TODO ... Get the current wear counter of all pages.
	}
	else
	{
		// Write the magic to the first byte.
		magic = EEPROM_MAGIC;
		_writeBytes(0, &magic, 1);

		// TODO ... Write the page count to the header.

		// TODO ... Write the pages info to the header.

		// TODO ... Clear the pages region.
	}
}
