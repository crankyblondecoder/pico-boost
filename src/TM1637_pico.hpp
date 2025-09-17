#ifndef TM1637_DISPLAY_H
#define TM1637_DISPLAY_H

#include <stdint.h>

/**
 * Display driver for TM1637 based multi-segment displays.
 *
 * The segment bits are as follows:
 *
 *     A
 *    ---
 * F |   | B
 *    -G-
 * E |   | C
 *    ---
 *     D
 *
 *  Where:
 *
 *     XGFEDCBA
 *   0b00000000
 */
class TM1637Display
{
	public:

		/**
		 * @param clk GPIO pin used for clock signal.
		 * @param dio GPIO pin used for data signal.
		 */
		TM1637Display(uint8_t clk, uint8_t dio);

		/** Clear all segments of the display. ie Switch it off. */
		void clear();

		/**
		 * Display chars in all 4 positions.
		 * The positions are from left to right, lowest to highest address respectively.
		 */
		void show(uint8_t data[4]);

		/**
		 * Display char in single position.
		 * The positions are from left to right, lowest to highest address respectively.
		 */
		void show(uint8_t position, uint8_t data);

		/** Set the brightness of the display. */
		void setBrightness(uint8_t brightness);

		/** Generate segment bitmap for a single digit (0-9). */
		uint8_t encodeDigit(unsigned digit);

		/**
		 * Encode a base 10 number into the given data buffer.
		 * @param number Number to encode.
		 * @param numDigits Number of digits to encode.
		 * @param startPosn Array index of lowest order digit.
		 * @param data Array of length 4 to encode number into. Lowest order digit goes into highest array address.
		 */
		void encodeNumber(unsigned number, unsigned numDigits, unsigned startPosn, uint8_t data[4]);

		/**
		 * Generate segment bitmap for a single characer.
		 * @note Not all characters can be encoded and only a single character will be encoded for any given case.
		 */
		uint8_t encodeAlpha(char character);

		/**
		 * Set the bits on the encoded segment bitmap to display the segments for the colon.
		 * @note Not every position in the display can show this.
		 */
		void encodeColon(uint8_t* encodedBitmap);

	private:

		/** Write start bit pattern. */
		void __start();

		/** Write stop bit pattern. */
		void __stop();

		/** Write a single byte. */
		uint8_t __writeByte(uint8_t data);

		/**
		 * Approximately wait a number of nanoseconds.
		 * @note This uses NOP based wasted cycles.
		 */
		void __wait_ns(int numNs);

		/** Clock pin. */
		uint8_t _clk;

		/** Data pin. */
		uint8_t _dio;

		/** Brightness value between 0x00 and 0x07 from not bright to full bright respectively. */
		uint8_t _brightness = 0x07;

		/** The number of CPU cycles for a quater clock. */
		uint32_t _cpuCyclesPerQuarterClock;

		/** The number of CPU cycles for a half clock. */
		uint32_t _cpuCyclesPerHalfClock;

		/** The number of CPU cycles for a full clock. */
		uint32_t _cpuCyclesPerClock;

		static const uint8_t _digitToSegment[];

		// Data command.
		static const uint8_t TM1637_CMD1 = 0x40;
		// Address command.
		static const uint8_t TM1637_CMD2 = 0xC0;
		// Display control command.
		static const uint8_t TM1637_CMD3 = 0x88;
};

#endif