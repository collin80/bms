/*
 * Config.h
 * Sets configuration options and stores EEPROM layout
 *
Copyright (c) 2015 Collin Kidder

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Arduino.h>

#ifndef CONFIG_H_
#define CONFIG_H_

#define CFG_BUILD_NUM	0x10

#define VIN_ADDR		0x48 // ADS1110-A0 the device address is 0x48  Voltage input
#define THERM_ADDR		0x4A // ADS1110-A2 the device address is 0x4A  Thermistor input

#define SWITCH_VBAT1_H	X10
#define SWITCH_VBAT2_L	X11
#define SWITCH_VBAT2_H	X9
#define SWITCH_VBAT3_L	X12
#define SWITCH_VBAT3_H	X13
#define SWITCH_VBAT4_L	X14
#define SWITCH_VBAT4_H	X15
#define SWITCH_VBATRTN	X16
#define SWITCH_THERM1	X17
#define SWITCH_THERM2	X18
#define SWITCH_THERM3	X19
#define SWITCH_THERM4	X20
#define CAN_TERM_1		X21
#define CAN_TERM_2		X22

//ADS1110 Config Defines
#define ADS_GAIN_1			0
#define ADS_GAIN_2			1
#define ADS_GAIN_4			2
#define ADS_GAIN_8			3
#define ADS_DATARATE_240	0
#define ADS_DATARATE_60		1 << 2
#define ADS_DATARATE_30		2 << 2
#define ADS_DATARATE_15		3 << 2
#define ADS_CONTINUOUS		0
#define ADS_SINGLE			1 << 4
#define ADS_START			1 << 7 //set to start a conversion in single mode
#define ADS_NOTREADY		1 << 7 //set if data is old or not ready yet 0 if new/good data
//This sets up 15 samples per second (16bit precision), 1x gain, and manual triggering of samples
//Also triggers a reading immediately.
#define ADS_CONFIG			ADS_DATARATE_15 | ADS_GAIN_1 | ADS_SINGLE | ADS_START

struct POLYNOMIAL
{
	float A, B, C, D; //coefficients for third order polynomial y = AX^3 + BX^2 + CX + D
	float adcToVolts; //scaling value to turn ADC value into volts for the above polynomial
};

struct EEPROMSettings {
	uint8_t version;
	uint32_t CANSpeed;
	boolean CAN_Enabled;

	int32_t cab300Address; //either 0x3C0 or 0x3C2 so far. Set to 0 if there isn't one installed in the car.

	uint16_t balanceThreshold; //how close in value the min and max sections can be without faulting - In millivolts
	
	//these two turn the ADC readings into volts/degrees.
	//Apparently first gen hardware actually has non-linearity for temp so handle specially.
	//Each ADC channel has its own multipler because the hardware used could have some differences in resistance
	float vMultiplier[4];
	POLYNOMIAL tMultiplier[4];	

	uint32_t vNominal[4]; //Nominal voltage of each quadrant in hundredths of a volt

	uint32_t maxPackAH; //number of amp hours in milliamp hours
	uint32_t currentPackAH; //number of amp hours in MICRO amp hours. This is still fine within a 32 bit unsigned int
	
	uint8_t logLevel; //Level of logging to output on serial line

	uint16_t valid; //stores a validity token to make sure EEPROM is not corrupt
};

#endif