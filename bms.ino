/*
EVTV BMS
2015

There are four thermistor inputs and four voltage inputs. The voltages are based on quarters of the pack
place five voltage taps - one at each end of the pack (+ and -) and three at the quadrant points in the middle
All five points on the pack should have 50k resistors to the battery. These resistors should also be fused
with something like a 1A fuse just in case a short develops. The resistors should be sturdy so that vibration
does not cause them to fail. Or, the alternative way is to use 30 to 26 ga wire so that a short just burns the wire
up in a wisp of smoke. That will work as well.
Feel like ordering from DigiKey?
A resistor that would work: 696-1513-ND
Fuse Holder: 02540202Z-ND
Fuse: F2452-ND

This device has voltage and temperate input but no current so there are two basic choices for current:
1. You can buy a CAB300 canbus connected current sensor. This code will accept input from such a sensor
2. You can also buy a JLD505 and use the current reporting capability of that device.

This device also does not drive outputs. For output driving you will have to use either a JLD505 or a GEVCU.
For instance, a SOC gauge would be nice. You should probably use the JLD505 for this purpose.
Another thing you might want to do is disable driving and/or throttle back acceleration when the batteries are getting low.
We have no output except canbus so the GEVCU will have to be notified of these things.

The bottom line is that maximum configurability and features will be had by using all three devices - JLD505, EVTV BMS, and GEVCU.
This is the golden combo. 

*/

#include <Arduino.h>
#include "SamNonDuePin.h" //Non-supported SAM3X pin library
#include <due_can.h>
#include <due_wire.h>
#include <Wire_EEPROM.h>
#include <DueTimer.h>


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

const uint8_t VBat[4][2] = {
								{SWITCH_VBAT1_H, SWITCH_VBAT2_L},{SWITCH_VBAT2_H, SWITCH_VBAT3_L}, 
								{SWITCH_VBAT3_H, SWITCH_VBAT4_L},{SWITCH_VBAT4_H,SWITCH_VBATRTN}
						   };

//There are three full readings per second so 32 entries is about 10 seconds worth of data
//Thus, the average of all these readings is a 10 second average of the pack performance
int16_t vReading[4][32];
int16_t tReading[4][32];
byte vReadingPos, tReadingPos;

struct EEPROMSettings {
	uint8_t version;
	uint32_t CANSpeed;
	boolean CAN_Enabled;

	int32_t cab300Address; //either 0x3C0 or 0x3C2 so far. Set to 0 if there isn't one installed in the car.

	int16_t balanceThreshold; //how close in value the min and max sections can be without faulting
	
	//these two turn the ADC readings into volts/degrees.
	//Apparently first gen hardware actually has non-linearity for temp so handle specially.
	//Each ADC channel has its own multipler because the hardware used could have some differences in resistance
	float vMultiplier[4];
	float tMultiplier[4]; 
	
	uint8_t logLevel; //Level of logging to output on serial line

	uint16_t valid; //stores a validity token to make sure EEPROM is not corrupt
};

EEPROMSettings settings;

/*Load settings from EEPROM. Fill out settings if not initialized yet*/
void loadEEPROM()
{
	EEPROM.read(0,settings);
	if (settings.valid != 0xDE)
	{
		settings.balanceThreshold = 0x200;
		settings.cab300Address = 0x3C0;
		settings.CANSpeed = 500000;
		settings.CAN_Enabled = true;
		settings.logLevel = 1;
		settings.tMultiplier[0] = 0.045f;
		settings.tMultiplier[1] = 0.045f;
		settings.tMultiplier[2] = 0.045f;
		settings.tMultiplier[3] = 0.045f;
		settings.vMultiplier[0] = 0.045f;
		settings.vMultiplier[1] = 0.045f;
		settings.vMultiplier[2] = 0.045f;
		settings.vMultiplier[3] = 0.045f;
		settings.valid = 0xDE;
		settings.version = 10;		
		EEPROM.write(0, settings);
	}
}

void setAllVOff()
{
  digitalWriteNonDue( SWITCH_VBAT1_H, LOW );
  digitalWriteNonDue( SWITCH_VBAT2_L, LOW );
  digitalWriteNonDue( SWITCH_VBAT2_H, LOW );
  digitalWriteNonDue( SWITCH_VBAT3_L, LOW );
  digitalWriteNonDue( SWITCH_VBAT3_H, LOW );
  digitalWriteNonDue( SWITCH_VBAT4_L, LOW );
  digitalWriteNonDue( SWITCH_VBAT4_H, LOW );
  digitalWriteNonDue( SWITCH_VBATRTN, LOW );
}

void setVEnable(uint8_t which)
{
	if (which > 3) return;
	setAllVOff();
	digitalWriteNonDue(VBat[which][0], HIGH);
	digitalWriteNonDue(VBat[which][1], HIGH);
}

void setAllThermOff()
{
  digitalWriteNonDue(SWITCH_THERM1, LOW );
  digitalWriteNonDue(SWITCH_THERM2, LOW);
  digitalWriteNonDue(SWITCH_THERM3, LOW );
  digitalWriteNonDue(SWITCH_THERM4, LOW );
}

void setThermActive(uint8_t which)
{
	if (which < SWITCH_THERM1) return;
	if (which > SWITCH_THERM4) return;
	setAllThermOff();
	digitalWriteNonDue(which, HIGH );
}

void canbusTermEnable()
{
  digitalWriteNonDue( CAN_TERM_1, HIGH );  
  digitalWriteNonDue( CAN_TERM_2, HIGH );

}

void canbusTermDisable()
{
  digitalWriteNonDue( CAN_TERM_1, LOW );  
  digitalWriteNonDue( CAN_TERM_2, LOW );
}

//ask for a reading to start. Currently we support 15 reads per second so one must wait
// at least 1000 / 15 = 67ms before trying to get the answer
void adsStartConversion(uint8_t addr)
{
  Wire.beginTransmission(addr); 
  Wire.write(ADS_CONFIG);
  Wire.endTransmission();
}

//returns true if data was waiting or false if it was not
bool adsGetData(byte addr, int16_t &value)
{
  byte status;

  //we ask to read the ADC value
  Wire.requestFrom(addr, 3); 
  value = Wire.read(); // first received byte is high byte of conversion data
  value <<= 8;
  value += Wire.read(); // second received byte is low byte of conversion data
  status = Wire.read(); // third received byte is the status register  
  if (status & ADS_NOTREADY) return false;
  return true;
}

void setupHardware()
{
	loadEEPROM();
  //Battery voltage inputs
  pinModeNonDue(SWITCH_VBAT1_H, OUTPUT );
  pinModeNonDue(SWITCH_VBAT2_L, OUTPUT );
  pinModeNonDue(SWITCH_VBAT2_H, OUTPUT );
  pinModeNonDue(SWITCH_VBAT3_L, OUTPUT );
  pinModeNonDue(SWITCH_VBAT3_H, OUTPUT );
  pinModeNonDue(SWITCH_VBAT4_L, OUTPUT );
  pinModeNonDue(SWITCH_VBAT4_H, OUTPUT );
  pinModeNonDue(SWITCH_VBATRTN, OUTPUT );
  setAllVOff();

  //Thermistor inputs
  pinModeNonDue(SWITCH_THERM1, OUTPUT );
  pinModeNonDue(SWITCH_THERM2, OUTPUT );
  pinModeNonDue(SWITCH_THERM3, OUTPUT );
  pinModeNonDue(SWITCH_THERM4, OUTPUT );
  setAllThermOff();

  pinModeNonDue(CAN_TERM_1, OUTPUT );  
  pinModeNonDue(CAN_TERM_2, OUTPUT );
  canbusTermDisable();

  if (settings.CAN_Enabled)
  {
	Can0.begin(settings.CANSpeed, 255); //no enable pin
	if (settings.cab300Address > 0) Can0.watchFor(settings.cab300Address); //allow through only this address for now
  }
}

//periodic tick that handles reading ADCs and doing other stuff that happens on a schedule.
//There are four voltage readings and four thermistor readings and they are on separate
//ads chips so each can be read every cycle which means it takes exactly 320ms to read the whole pack
//Or, the pack is fully characterized three times per second. Not too shabby!
void timerTick()
{
	static byte vNum, tNum; //which voltage and thermistor reading we're on

	int16_t readValue;
	
	//read the values from the previous cycle
	//if there is a problem we won't update the values stored
	if (adsGetData(VIN_ADDR, readValue)) 
	{
		vReading[vNum][vReadingPos] = readValue;
		vReadingPos = (vReadingPos + 1) & 31;
	}

	if (adsGetData(THERM_ADDR, readValue)) 
	{
		tReading[vNum][tReadingPos] = readValue;
		tReadingPos = (tReadingPos + 1) & 31;
	}

	//Set up for the next set of readings
	setVEnable(vNum++);
	adsStartConversion(VIN_ADDR);
	setThermActive(SWITCH_THERM1 + tNum++);
	adsStartConversion(THERM_ADDR);
	
	//constrain these back to valid range 0-3
	vNum &= 3;
	tNum &= 3;

	int32_t vAccum[4], tAccum[4];
	int16_t vHigh = -10000, vLow = 30000;

	for (int y = 0; y < 4; y++)
	{
		vAccum[y] = 0;
		tAccum[y] = 0;
		for (int x = 0; x < 32; x++)
		{
			vAccum[y] += vReading[y][x];
			tAccum[y] += tReading[y][x];
		}
		//now get averages.
		vAccum[y] /= 32;
		tAccum[y] /= 32;
		if (vAccum[y] > vHigh) vHigh = vAccum[y];
		if (vAccum[y] < vLow) vLow = vAccum[y];
	}

	//Now, if the difference between vLow and vHigh is too high then there is a serious pack balancing problem
	//and we should let someone know
	if (vHigh - vLow > settings.balanceThreshold)
	{
	}
	SerialUSB.print(vAccum[0]);
	SerialUSB.print(vAccum[1]);
	SerialUSB.print(vAccum[2]);
	SerialUSB.print(vAccum[3]);
	SerialUSB.println();
}

void setup()
{
  Wire.begin(); // wake up I2C bus
  SerialUSB.begin(115200);

  setupHardware();

  Timer3.attachInterrupt(timerTick);
  Timer3.start(80000); //trigger every 80ms
}

void loop()
{
  
}
