#include "Eeprom.hpp"

Eeprom::~Eeprom()
{
	if(_pages) delete[] _pages;
	if(_curWearIndexes) delete[] _curWearIndexes;
	if(_curWearPage) delete[] _curWearPage;
}

Eeprom::Eeprom(unsigned size, EepromPage* pages, uint8_t pageCount)
{
	_eepromSize = size;
	_pages = 0;
	_pageCount = pageCount;

	_curWearIndexes = 0;
	_curWearPage = 0;

	// Total amount allocated to pages region. Includes page counters.
	unsigned totalAlloc;

	if(pageCount > 0)
	{
		// Copy the given pages and setup the current wear index arrays.

		_pages = new EepromPage[pageCount];

		_curWearIndexes = new uint16_t[pageCount];
		_curWearPage = new uint16_t[pageCount];

		for(unsigned index = 0; index < pageCount; index++)
		{
			_pages[index].pageSize = pages[index].pageSize;
			_pages[index].wearCount = pages[index].wearCount & 0x7FFF;

			totalAlloc += _pages[index].wearCount * (_pages[index].pageSize + 2);

			_curWearIndexes[index] = 0;
		}
	}

	// First bytes (header) of EEPROM are the magic number followed by the page count and then the pages info.
	// If any existing header doesn't match what is expected then the header is re-written and the pages region cleared.

	bool headerMatches = false;

	uint8_t magic;
	_readBytes(0, &magic, 1);

	if(magic == EEPROM_MAGIC)
	{
		// Check for header matching expected pages.

		uint8_t headerPageCount;
		_readBytes(1, &headerPageCount, 1);

		if(headerPageCount == pageCount)
		{
			uint8_t headerPageSize;
			uint16_t headerWearCount;

			headerMatches = true;

			uint32_t curAddr = EEPROM_PAGE_COUNT_ADDR + 1;

			for(unsigned index = 0; index < pageCount; index++)
			{
				_readBytes(curAddr++, &headerPageSize, 1);
				// This is fine as long a everything is little endian.
				_readBytes(curAddr, (uint8_t*)&headerWearCount, 2);

				curAddr += 2;

				// Compare to expected page.
				if(headerPageSize != _pages[index].pageSize || headerWearCount != _pages[index].wearCount)
				{
					headerMatches = false;
					break;
				}
			}
		}
	}

	if(headerMatches)
	{
		// TODO ... Get the current wear index of all pages.
		for(unsigned index = 0; index < pageCount; index++)
		{
			uint8_t pageSize = _pages[index].pageSize;
			uint16_t wearCount = _pages[index].wearCount;

			uint32_t pageAllocSize = pageSize * (wearCount + 2);


		}
	}
	else
	{
		// Write the magic to the first byte.
		magic = EEPROM_MAGIC;
		_writeBytes(0, &magic, 1);

		// Write the page count to the header.
		_writeBytes(EEPROM_PAGE_COUNT_ADDR, &_pageCount, 1);

		// Write the pages info to the header.
		uint32_t curHeaderAddr = EEPROM_PAGE_COUNT_ADDR + 1;

		// Start of the actual page that corresponds to the current page header being written.
		uint32_t curPageAddr = curHeaderAddr + pageCount * 3;

		for(unsigned index = 0; index < pageCount; index++)
		{
			uint8_t pageSize = _pages[index].pageSize;
			uint16_t wearCount = _pages[index].wearCount;

			_writeBytes(curHeaderAddr++, &pageSize, 1);
			// This is fine as long a everything is little endian.
			_writeBytes(curHeaderAddr, (uint8_t*)&wearCount, 2);

			// Clear the pages region.
			uint32_t pageAllocSize = pageSize * (wearCount + 2);
			_clear(0xFF, curPageAddr, pageAllocSize);

			// Calc next page start.
			curPageAddr += pageAllocSize;
		}
	}
}

void Eeprom::clear(uint8_t value, unsigned start, unsigned count)
{
	_clear(value, start, count);
}
