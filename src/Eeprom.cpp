#include <stdio.h>

#include "Eeprom.hpp"

Eeprom::~Eeprom()
{
	if(_pages) delete[] _pages;
	if(_pageInstances) delete[] _pageInstances;
}

Eeprom::Eeprom(unsigned size, EepromPage* pages, uint8_t pageCount)
{
	_eepromSize = size;
	_pages = 0;
	_pageCount = pageCount;

	_pageInstances = 0;

	if(pageCount > 0)
	{
		// Copy the given pages and setup the page instances.

		_pages = new EepromPage[pageCount];

		_pageInstances = new EepromPageInstance[pageCount];

		// Point to start of first region after the page header.
		uint32_t curPageRegionStartAddr = EEPROM_PAGE_COUNT_ADDR + pageCount * 3 + 1;

		for(unsigned index = 0; index < pageCount; index++)
		{
			_pages[index].pageSize = pages[index].pageSize;
			_pages[index].wearCount = pages[index].wearCount & 0x7FFF;

			_pageInstances[index].physPageIndex = 0;
			_pageInstances[index].wearIndex = 0;
			_pageInstances[index].regionStartAddress = curPageRegionStartAddr;

			// Takes into account page instance wear index.
			curPageRegionStartAddr += _pages[index].wearCount * (_pages[index].pageSize + 2);
		}

		// This should now point to the start of the non-page region.
		_nonPageRegionStartAddress = curPageRegionStartAddr;
	}
}

bool Eeprom::writeBytes(uint32_t startAddr, uint8_t* values, unsigned count)
{
	return _writeBytes(startAddr, values, count);
}

bool Eeprom::readBytes(uint32_t startAddr, uint8_t* buffer, unsigned count)
{
	return _readBytes(startAddr, buffer, count);
}

void Eeprom::_init()
{
	// First bytes (header) of EEPROM are the magic number followed by the page count and then the pages info.
	// If any existing header doesn't match what is expected then the header is re-written and the pages region cleared.

	bool headerMatches = false;

	uint8_t magic;
	bool okay = _readBytes(0, &magic, 1);

	if(!okay)
	{
		// Try again, just one more time.
		okay = _readBytes(0, &magic, 1);
	}

	// Simply abort rather than trying to overwrite.
	if(!okay) return;

	// Current EEPROM byte address being processed.
	uint32_t curAddr = 0;

	if(magic == EEPROM_MAGIC)
	{
		// Check for header matching expected pages.

		uint8_t headerPageCount;
		_readBytes(1, &headerPageCount, 1);

		if(headerPageCount == _pageCount)
		{
			uint8_t headerPageSize;
			uint16_t headerWearCount;

			headerMatches = true;

			curAddr = EEPROM_PAGE_COUNT_ADDR + 1;

			for(unsigned index = 0; index < _pageCount; index++)
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
		// Assume curAddr already points to the first page instance of the first page descr.

		// Get the current wear index of all pages.

		for(unsigned descrIndex = 0; descrIndex < _pageCount; descrIndex++)
		{
			uint16_t wearCount = _pages[descrIndex].wearCount;

			// Add extra data per page instance for 16bit wear index.
			uint32_t pageInstanceSize = _pages[descrIndex].pageSize + 2;

			uint16_t nextWearIndex;
			uint16_t prevWearIndex = 0;

			curAddr = _pageInstances[descrIndex].regionStartAddress;

			for(unsigned instIndex = 0; instIndex < wearCount; instIndex++)
			{
				_readBytes(curAddr + instIndex * pageInstanceSize, (uint8_t*)&nextWearIndex, 2);

				// Wear page is blank. End of search.
				if(nextWearIndex == 0xFFFF) break;

				// Next wear index must be one more than the previous for the chain of indexes to be complete.
				// A break in the chain indicates the end of the current wear indexes.

				if(prevWearIndex == 0 || nextWearIndex == prevWearIndex + 1)
				{
					// Found more recent page instance.
					_pageInstances[descrIndex].physPageIndex = instIndex;
					_pageInstances[descrIndex].wearIndex = nextWearIndex;
				}
				else
				{
					break;
				}

				prevWearIndex = nextWearIndex;
			}

			// Jump to next page region.
			curAddr += pageInstanceSize * wearCount;
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

		// Start of the actual physical page instances region that corresponds to a particular page descr.
		// A page descriptor is 3 bytes long.
		uint32_t curPageRegionAddr = curHeaderAddr + _pageCount * 3;

		for(unsigned index = 0; index < _pageCount; index++)
		{
			uint8_t pageSize = _pages[index].pageSize;
			uint16_t wearCount = _pages[index].wearCount;

			_writeBytes(curHeaderAddr++, &pageSize, 1);
			// This is fine as long a everything is little endian.
			_writeBytes(curHeaderAddr, (uint8_t*)&wearCount, 2);

			// Move onto next header entry.
			curHeaderAddr += 2;

			// Clear the pages region.
			uint32_t pageRegionAllocSize = (pageSize + 2) * wearCount;
			_clear(0xFF, curPageRegionAddr, pageRegionAllocSize);

			// Calc next page instance region start.
			curPageRegionAddr += pageRegionAllocSize;
		}
	}

	_pagesInitialised = true;
}

void Eeprom::clear(uint8_t value, unsigned start, unsigned count)
{
	_clear(value, start, count);
}

bool Eeprom::readPage(uint8_t pageId, uint8_t* page)
{
	// Page subsystem not initialised or no pages have been written yet for the given page id.
	if(!_pagesInitialised || _pageInstances[pageId].wearIndex == 0) return false;

	uint8_t pageSize = _pages[pageId].pageSize;

	// Get pages start byte address. The +2 is to account for the wear index.
	uint32_t pageInstanceStartAddr = _pageInstances[pageId].regionStartAddress + (pageSize + 2) *
		_pageInstances[pageId].physPageIndex + 2;

	return _readBytes(pageInstanceStartAddr, page, pageSize);
}

bool Eeprom::writePage(uint8_t pageId, uint8_t* pageData)
{
	if(!_pagesInitialised) return false;

	// Get next wear index to write. Rely on unsigned 16 bit integer rolling over to 0 once maxed out.
	uint16_t nextWearIndex = _pageInstances[pageId].wearIndex + 1;

	// Wear index can't be 0 because that is reserved for "no pages present".
	if(nextWearIndex == 0) nextWearIndex++;

	// Find the next page index to write to. Check for "no pages written yet".
	uint16_t nextPageIndex = _pageInstances[pageId].wearIndex == 0 ? 0 : _pageInstances[pageId].physPageIndex + 1;

	// Check for whether to go back to writing first wear page.
	if(nextPageIndex >= _pages[pageId].wearCount) nextPageIndex = 0;

	uint8_t pageSize = _pages[pageId].pageSize;

	// Get byte address to write to.
	uint32_t pageInstanceStartAddr = _pageInstances[pageId].regionStartAddress + (pageSize + 2) * nextPageIndex;

	// Write wear index.
	bool okay = _writeBytes(pageInstanceStartAddr, (uint8_t*)&nextWearIndex, 2);

	if(okay)
	{
		// Write page data.
		okay = _writeBytes(pageInstanceStartAddr + 2, pageData, pageSize);

		_pageInstances[pageId].wearIndex = nextWearIndex;
		_pageInstances[pageId].physPageIndex = nextPageIndex;
	}

	return okay;
}

uint32_t Eeprom::getNonPageRegionStartAddress()
{
	return _nonPageRegionStartAddress;
}

bool Eeprom::verifyMetaData(EepromPage* pages, uint8_t pageCount)
{
	uint8_t buffer[2];

	// Read the magic to the first byte.
	_readBytes(0, buffer, 1);
	bool verified = buffer[0] = EEPROM_MAGIC;

	if(verified)
	{
		// Read the page count from the header.
		_readBytes(EEPROM_PAGE_COUNT_ADDR, buffer, 1);
		verified = buffer[0] == EEPROM_PAGE_COUNT_ADDR;
	}

	uint32_t curHeaderAddr = EEPROM_PAGE_COUNT_ADDR + 1;

	for(unsigned index = 0; index < _pageCount; index++)
	{
		uint8_t pageSize = _pages[index].pageSize;
		uint16_t wearCount = _pages[index].wearCount;

		_readBytes(curHeaderAddr, buffer, 1);

		verified = buffer[0] == pageSize;
		if(!verified) break;

		_readBytes(curHeaderAddr + 1, buffer, 2);

		verified = *((uint16_t*)buffer) == wearCount;
		if(!verified) break;

		// Move onto next header entry.
		curHeaderAddr += 3;
	}

	return verified;
}