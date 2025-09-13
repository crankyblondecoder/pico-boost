// GPIO Allocation

/** The GPIO pin used for i2c bus 0 SDA (data) line. EEPROM Usage. */
#define I2C_BUS0_SDA_GPIO 12

/** The GPIO pin used for i2c bus 0 SCL (clock) line. EEPROM Usage. */
#define I2C_BUS0_SCL_GPIO 13

/** Navigation button - middle. */
#define NAV_BTN_MIDDLE 2

/** Navigation button - right. */
#define NAV_BTN_RIGHT 3

/** Navigation button - left. */
#define NAV_BTN_LEFT 4

/** Navigation button - back. */
#define NAV_BTN_BACK 5

/** Navigation button - forward. */
#define NAV_BTN_FORWARD 6

/** Clock line to the display. */
#define DISPLAY_CLOCK_GPIO 15

/** Data line to the display. */
#define DISPLAY_DATA_GPIO 14

/** Switched preset index select input. ie When this is pulled high a particular preset index is selected. */
#define PRESET_INDEX_SELECT_GPIO 18

/** GPIO That channel A of the PWM slice used to control the solenoid. */
#define CONTROL_SOLENOID_CHAN_A_GPIO 20 // Slice 2

/** Unused Channel GPIO from setting up PWM slice 2. ie Do not use for anything other than PWM. */
#define UNUSED_PWM_SLICE2_CHAN_B_GPIO 21

/** Min brightness */
#define MIN_BRIGHTNESS_GPIO 22

/** ADC Channel 0. */
#define ADC_0_MAP_SENSOR 26

/** ADC Channel 1. */
#define ADC_1_UNUSED 27

/** ADC Channel 2. */
#define ADC_2_UNUSED 28