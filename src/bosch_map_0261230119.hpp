#ifndef BOSCH_MAP_0261230119_H
#define BOSCH_MAP_0261230119_H

#include <stdint.h>

/**
 * Bosch 0261230119 map sensor.
 */
class bosch_map_0261230119
{
	public:

		/**
		 * Create new bosch 0261230119 map sensor.
		 * @param adcInput ADC input number (Pico I has 0,1 and 2).
		 */
		bosch_map_0261230119(unsigned adcInput);

		/**
		 * Read the map sensor and return the value in kpa.
		 */
		double readKpa();

		/**
		 * Read the map sensor and return the value in psi.
		 */
		double readPsi();

	private:

		/**
		 * Read the map sensor and return the value in kpa.
		 */
		double __readKpa();

		/** ADC Input to read from. */
		unsigned _adcInput;

		/** The c0 constant from the datasheet. */
		double _bosch_map_c0 = 5.4 / 280.0;

		/** The c1 constant from the datasheet. */
		double _bosch_map_c1 = 0.85 / 280.0;

};

#endif