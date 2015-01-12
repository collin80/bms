/*
EVTV BMS
2015

There are four thermistor inputs and four voltage inputs. The voltages are based on quarters of the pack
place five voltage taps - one at each end of the pack (+ and -) and three at the quadrant points in the middle
All five points on the pack should have 50k resistors to the battery. These resistors should also be fused
with something like a 1A fuse just in case a short develops. The resistors should be sturdy so that vibration
does not cause them to fail.
A resistor that would work: 696-1513-ND
Fuse Holder: 02540202Z-ND
Fuse: F2452-ND

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
#define ADS_START			1 << 7
#define ADS_READY			1 << 7

const uint8_t VBat[4][2] = {
								{SWITCH_VBAT1_H, SWITCH_VBAT2_L},{SWITCH_VBAT2_H, SWITCH_VBAT3_L}, 
								{SWITCH_VBAT3_H, SWITCH_VBAT4_L},{SWITCH_VBAT4_H,SWITCH_VBATRTN}
						   };
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

void setupHardware()
{
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
}

void setup()
{
  Wire.begin(); // wake up I2C bus
  SerialUSB.begin(115200);

  setupHardware();
}


void adsGetData(byte addr, byte &a, byte &b, byte &c)
{
  // move the register pointer back to the first register
  Wire.beginTransmission(addr); 
  Wire.write(0x0C);
  Wire.endTransmission();
  // now get the data from the ADS1110
  Wire.requestFrom(addr, 3); 
  a = Wire.read(); // first received byte is high byte of conversion data
  b = Wire.read(); // second received byte is low byte of conversion data
  c = Wire.read(); // third received byte is the status register
}

void loop()
{
  
}
