#ifndef BOSCH_MAP_0261230119_H
#define BOSCH_MAP_0261230119_H

#include <stdint.h>

/** The GPIO pin used to get VSYS voltage via ADC. */
#define VSYS_REF_GPIO 29

/** The ADC channel used to get VSYS voltage. */
#define VSYS_REF_CHANNEL 3

/**
 * Bosch 0261230119 map sensor.
 */
class bosch_map_0261230119
{
	public:

		virtual ~bosch_map_0261230119();

		/**
		 * Create new bosch 0261230119 map sensor.
		 * @param adcInput ADC input number (Pico I has 0, 1 and 2 as external pins).
		 * @param avgCount The number of read values to average the result over.
		 */
		bosch_map_0261230119(unsigned adcInput, unsigned avgCount);

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

		/**
		 * Read the voltage on VSYS.
		 */
		double __readVSys();

		/**
		 * Calculate the raw averge values across all raw latched data.
		 */
		void __calcRawAvgVals();

		/**
		 * Total number of raw values that can be kept.
		 */
		unsigned _avgCount;

		/** Raw ADC pressure values that have been read. */
		uint16_t* _rawPressureVals;

		/** Raw ADC VSys values that have been read. */
		uint16_t* _rawVSysVals;

		/** The current position that raw vals are written into. */
		unsigned _curRawValPosn = 0;

		/** The last calculated raw pressure average. */
		unsigned _lastRawPressureAvg;

		/** The last calculated raw VSys average. */
		unsigned _lastRawVSysAvg;

		/** ADC Input to read from. */
		unsigned _adcInput;

		/** The c0 constant from the datasheet. */
		double _bosch_map_c0 = 5.4 / 280.0;

		/** The c1 constant from the datasheet. */
		double _bosch_map_c1 = 0.85 / 280.0;

		/** Whether the VSYS ref voltage ADC reading is setup. */
		static bool _vSysRef_setup;

		/** Scale factor used for ADC readings. Multiply this by the ADC reading to get the voltage. */
		static double _adcScale;
};

#endif