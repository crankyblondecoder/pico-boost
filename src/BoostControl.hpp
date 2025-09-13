#ifndef PICO_BOOST_CONTROL_H
#define PICO_BOOST_CONTROL_H

#include "pico/time.h"

#include "BoostControlParameters.hpp"
#include "BoschMap_0261230119.hpp"
#include "gpioAlloc.hpp"
#include "PicoAdcReader.hpp"
#include "PicoPwm.hpp"

/** Standard atmospheric pressure in Pascals. */
#define STD_ATM_PRESSURE 101325

/** Conversion factor of KPa to PSI. */
#define KPA_TO_PSI 0.145038

/** Frequency of control solenoid. */
#define CONTROL_SOLENOID_FREQ 30

/** Gate state to use when disabling PWM. */
#define CONTROL_SOLENOID_DISABLE_GATE_STATE false

/**
 * De-energise/energise hysteresis to stop rapid flucations with enable/disable of PWM around that point.
 * In KPA, scaled by 1000.
 */
#define CONTROL_DE_ENERGISE_HYSTERESIS 5000

/** Time in seconds over which the PID integral term is summed. */
#define CONTROL_PID_INTEG_SUM_TIME 0.5

/**
 * Class to control a single instance of a boost control solenoid.
 * @note This _only_ controls duty cycle where an increase in duty increases boost. ie The solenoid de-energizing takes the
 *       system back to waste gate spring pressure.
 */
class BoostControl
{
	public:

		virtual ~BoostControl();

		BoostControl();

		/**
		 * Get whether boost control is ready (initialised).
		 */
		bool ready();

		/**
		 * Get all boost control parameters.
		 * @param params Populate (copy into) this with current parameters.
		 */
		void getParameters(BoostControlParameters* params);

		/**
		 * Set all boost control parameters.
		 * @note This doesn't copy but uses the supplied struct directly.
		 * @param params Parameter data to use.
		 */
		void setParameters(BoostControlParameters* params);

		/**
		 * Set boost control parameters to defaults.
		 * @param params Parameter data to set to defaults.
		 */
		void populateDefaultParameters(BoostControlParameters* params);

		/** Polling pass for boost control. */
		void poll();

		/** Get whether the boost control solenoid is energised. */
		bool isEnergised();

		/** Get whether maximum boost has been reached. */
		bool isMaxBoostReached();

		/**
		 * Get the current boost value, relative to std atm, as measured from the MAP sensor. In kPa, scaled by 1000.
		 * Can be negative for below std atm.
		 */
		int getKpaScaled();

		/**
		 * Get the current boost value, relative to std atm, as measured from the MAP sensor. In PSI, scaled by 10.
		 * Can be negative for below std atm.
		 */
		int getPsiScaled();

		/**
		 * Get the maximum boost, relative to std atm. In kPa, scaled by 1000.
		 */
		unsigned getMaxKpaScaled();

		/**
		 * Alter the maximum boost, relative to std atm, by adjusting it by a delta.
		 * @param maxBoostKpaScaledDelta Delta, scaled by 1000.
		 */
		void alterMaxKpaScaled(int maxBoostKpaScaledDelta);

		/**
		 * Get the de-energise boost, relative to std atm. In kPa, scaled by 1000.
		 */
		unsigned getDeEnergiseKpaScaled();

		/**
		 * Alter the de-energise boost, relative to std atm, by adjusting it by a delta.
		 * @param deEnergiseKpaScaledDelta Delta, scaled by 1000.
		 */
		void alterDeEnergiseKpaScaled(int deEnergiseKpaScaledDelta);

		/**
		 * Get the pid active boost, relative to std atm. In kPa, scaled by 1000.
		 */
		unsigned getPidActiveKpaScaled();

		/**
		 * Alter the pid active boost, relative to std atm, by adjusting it by a delta.
		 * @param delta Delta, scaled by 1000.
		 */
		void alterPidActiveKpaScaled(int delta);

		/**
		 * Get the PID proportional constant. Scaled by 1000.
		 */
		unsigned getPidPropConstScaled();

		/**
		 * Alter the PID proportional constant by adjusting it by a delta.
		 * @param pidPropConstScaledDelta Delta, scaled by 1000.
		 */
		void alterPidPropConstScaled(int pidPropConstScaledDelta);

		/**
		 * Get the PID integration constant. Scaled by 1000.
		 */
		unsigned getPidIntegConstScaled();

		/**
		 * Alter the PID integration constant by adjusting it by a delta.
		 * @param pidIntegConstScaledDelta Delta, scaled by 1000.
		 */
		void alterPidIntegConstScaled(int pidIntegConstScaledDelta);

		/**
		 * Get the PID derivative constant. Scaled by 1000.
		 */
		unsigned getPidDerivConstScaled();

		/**
		 * Alter the PID derivative constant by adjusting it by a delta.
		 * @param pidDerivConstScaledDelta Delta, scaled by 1000.
		 */
		void alterPidDerivConstScaled(int pidDerivConstScaledDelta);

		/**
		 * Get maximum duty cycle the solenoid can be set at. Scaled by 10.
		 */
		unsigned getMaxDutyScaled();

		/**
		 * Alter maximum duty cycle the solenoid can be set at.
		 * @param maxDutyDelta Delta, scaled by 10.
		 */
		void alterMaxDutyScaled(int maxDutyDelta);

		/**
		 * Get zero point duty cycle of the solenoid. Scaled by 10.
		 */
		unsigned getZeroPointDutyScaled();

		/**
		 * Alter zero point duty cycle of the solenoid.
		 * @param dutyDelta Delta, scaled by 10.
		 */
		void alterZeroPointDutyScaled(int dutyDelta);

		/**
		 * Get the current boost control solenoid duty cycle, in %. Scaled by 10.
		 */
		unsigned getCurrentDutyScaled();

		/**
		 * Run a test cycle with the solenoid valve.
		 */
		void testSolenoid();

		/**
		 * Read the voltage supplied to the map sensor.
		 */
		double mapReadSupplyVoltage();

		/**
		 * Read the current map sensor voltage.
		 */
		double mapReadSensorVoltage();

	protected:

	private:

		/** Whether this has been initialised. */
		bool _initialised = false;

		/** Bosch map sensor to read current turbo pressure from. */
		BoschMap_0261230119* _mapSensor = 0;

		/** The ADC reader to read the system (supply) voltage. */
		PicoAdcReader* _vsysRefAdc = 0;

		/** The current boost parameters. */
		BoostControlParameters _curParams;

		/**
		 * The current boost MAP sensor reading, scaled by 1000 so a float isn't required and 3 decimal places are used.
		 * It is done this way because read/write on 32bit numbers are atomic on the RP2040.
		 * @note This is an absolute pressure reading.
		 */
		uint32_t _mapKpaScaled = 0;

		/** Whether the solenoid is currently energised. ie PWM is active. */
		bool _energised = false;

		/** Whether solenoid is being controlled by the PID algorithm. */
		bool _pidActive = false;

		/** Previous error for the PID algorithm. */
		float _pidPrevError = 0;

		/** Current integral value for the PID algorithm. This is _not_ multiplied by the constant. */
		float _pidInteg = 0;

		/** The last time the PID algorithm was processed. */
		absolute_time_t _lastPidProcTime;

		/** The next time the boost map sensor is latched (reads and stores the current value of the sensor) */
		absolute_time_t _nextBoostLatchTime;

		/** The next time the current boost value is read. This is typically averaged across several latched values. */
		absolute_time_t _nextBoostReadTime;

		/** The last time the solenoid duty cycle was processed. */
		absolute_time_t _lastSolenoidProcTime;

		/** PWM control. Assume N Channel Mosfet (IRLZ34N) is being used and the gate must be pulled to ground. */
		PicoPwm* _pwmControl;

		/** Whether test mode is currently active. */
		bool _testMode = false;

		/** Process the control solenoid parameters and energise it accordingly. */
		void __processControlSolenoid();

		/** Set percentage duty cycle. */
		void __setSolenoidDuty(float duty);

		/** Enable the boost control solenoid. */
		void __enableSolenoid();

		/** Disable the boost control solenoid. */
		void __disableSolenoid();

		/**
		 * Set the maximum boost, relative to std atm. In kPa, scaled by 1000.
		 */
		void __setMaxKpaScaled(unsigned maxKpaScaled);

		/**
		 * Set the pid active boost, relative to std atm. In kPa, scaled by 1000.
		 */
		void __setPidActiveKpaScaled(unsigned kpaScaled);

		/**
		 * Set the de-energise boost, relative to std atm. In kPa, scaled by 1000.
		 */
		void __setDeEnergiseKpaScaled(unsigned kpaScaled);

		/**
		 * Set the PID proportional constant. Scaled by 1000.
		 */
		void __setPidPropConstScaled(unsigned pidPropConstScaled);

		/**
		 * Set the PID integration constant. Scaled by 1000.
		 */
		void __setPidIntegConstScaled(unsigned pidIntegConstScaled);

		/**
		 * Set the PID derivative constant. Scaled by 1000.
		 */
		void __setPidDerivConstScaled(unsigned pidDerivConstScaled);

		/**
		 * Set maximum duty cycle the solenoid can be set at. Scaled by 10.
		 */
		void __setMaxDutyScaled(unsigned maxDuty);

		/**
		 * Set zero point duty cycle of the solenoid. Scaled by 10.
		 */
		void __setZeroPointDutyScaled(unsigned zeroPointDuty);
};

#endif