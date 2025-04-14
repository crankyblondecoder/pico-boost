#ifndef BOSCH_MAP_0261230119_H
#define BOSCH_MAP_0261230119_H

/** KPa to PSI conversion factor. */
#define KPA_TO_PSI 0.145038

#include <stdint.h>

#include "PicoAdcReader.hpp"

/**
 * Bosch 0261230119 map sensor.
 * @note It is assumed that the Pico's ADC reference is shunted to 3.0V.
 */
class BoschMap_0261230119
{
	public:

		virtual ~BoschMap_0261230119();

		/**
		 * @param adcInput ADC input number (Pico I has 0, 1 and 2 as external pins).
		 * @param vSysAdcReader The ADC reader that provides VSys voltage. Not owned by this.
		 */
		BoschMap_0261230119(unsigned adcInput, PicoAdcReader* vSysAdcReader);

		/**
		 * Latch the current raw map sensor data.
		 * This will be averaged with other data to get a current value.
		 */
		void latch();

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

		/** The c0 constant from the datasheet. */
		double _bosch_map_c0 = 5.4 / 280.0;

		/** The c1 constant from the datasheet. */
		double _bosch_map_c1 = 0.85 / 280.0;

		/** Ratio of voltage divider used to convert MAP sensor 5V output to VRef range. */
		static double _voltageDividerRatio;

		/** Pico ADC reader for the MAP sensor input. */
		PicoAdcReader* _picoAdcReader;

		/** Pico ADC reader for VSys. */
		PicoAdcReader* _vSysAdcReader;
};

#endif