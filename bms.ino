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
#include "config.h"
#include "SerialConsole.h"
#include "i2c_adc.h"

EEPROMSettings settings;
SerialConsole	console;
ADCClass* adc;

/*Load settings from EEPROM. Fill out settings if not initialized yet*/
void loadEEPROM()
{
	EEPROM.read(0,settings);
	if (settings.valid != 0xDE || settings.version != 11)
	{
		settings.balanceThreshold = 0x200;
		settings.cab300Address = 0x3C0;
		settings.CANSpeed = 500000;
		settings.CAN_Enabled = true;
		settings.logLevel = 1;
		for (int x = 0; x < 4; x++) 
		{ 
			settings.tMultiplier[x].adcToVolts = 0.0000625609f;
			settings.tMultiplier[x].A = 1.8794f;
			settings.tMultiplier[x].B = 2.561f;
			settings.tMultiplier[x].C = 17.433f;
			settings.tMultiplier[x].D = 22.679;
		}
		//voltage multipler calculated based on 100k battery resistance and 2k resistor on board.
		//The actual multiplier will be a little bit off from this but this value is a good start.
		settings.vMultiplier[0] = 0.001593752f;
		settings.vMultiplier[1] = 0.001593752f;
		settings.vMultiplier[2] = 0.001593752f;
		settings.vMultiplier[3] = 0.001593752f;
		settings.valid = 0xDE;
		settings.version = 10;		
		EEPROM.write(0, settings);
	}
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
	loadEEPROM();

  pinModeNonDue(CAN_TERM_1, OUTPUT );  
  pinModeNonDue(CAN_TERM_2, OUTPUT );
  canbusTermEnable();

  if (settings.CAN_Enabled)
  {
	Can0.begin(settings.CANSpeed, 255); //no enable pin
	if (settings.cab300Address > 0) Can0.watchFor(settings.cab300Address); //allow through only this address for now
  }
 

  Can0.setGeneralCallback(canbusRX);
}

void canbusRX(CAN_FRAME *frame)
{
	if (frame->id == settings.cab300Address)
	{
		if (frame->data.byte[4] & 1) //ERROR!
		{						
			byte faultCode = frame->data.byte[4] >> 1;
			switch (faultCode)
			{
			case 0x41:
				Logger::error("CAB300 - Error on dataflash CRC");				
				break;
			case 0x42:
				Logger::error("CAB300 - Fluxgate running high freq");
				break;
			case 0x43:
				Logger::error("CAB300 - Fluxgate not oscillating");
				break;
			case 0x44:
				Logger::error("CAB300 - CAB entered failsafe mode");
				break;
			case 0x46:
				Logger::error("CAB300 - Signal not avail");
				break;
			case 0x47:
				Logger::error("CAB300 - Bridge voltage protection");
				break;
			default:
				Logger::error("CAB300 - Something bad happened");
				break;
			}
		}
		else
		{
			int64_t tempCurr;
			tempCurr = frame->data.byte[0] << 24;
			tempCurr += frame->data.byte[1] << 16;
			tempCurr += frame->data.byte[2] << 8;
			tempCurr += frame->data.byte[3];
			int32_t currentReading = (int32_t)(tempCurr - 0x80000000);
			float currentValue = currentReading / 1000.0f;
			Logger::debug("CAB300 - Current %f", currentValue);

		}
	}
}

void setup()
{
  Wire.begin(); // wake up I2C bus
  SerialUSB.begin(115200);

  setupHardware();

  adc = ADCClass::getInstance();
  adc->setup();

  console.printMenu();
}

void loop()
{
	if (SerialUSB.available()) 
	{
		console.rcvCharacter((uint8_t)SerialUSB.read());
	}
}
