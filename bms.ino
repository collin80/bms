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
#include "CanbusHandler.h"

EEPROMSettings settings;
STATUS status;
SerialConsole	console;
CANBusHandler *cbHandler;
ADCClass *adc;
uint32_t lastWrite;

bool firstConnect = true;
bool needInitialConfig = false;

/*Load settings from EEPROM. Fill out settings if not initialized yet*/
void loadEEPROM()
{
	EEPROM.read(0,settings);
	if (settings.valid != 0xDE || settings.version != CFG_EEPROM_VER)
	{
		settings.balanceThreshold = 0x200; //512 mv
		settings.currentPackAH  = 0;
		settings.maxPackAH = 0; //there's no way to know...
		settings.highTempThresh = 800; //80C - HOT!
		settings.lowTempThresh = -50; //-5C - Chilly!
		settings.highThreshold = 3850; //3.850 volts
		settings.lowThreshold = 2700; //2.700 volts		
		
		settings.bmsBaseAddress = 0x606;
		settings.cab300Address = 0x3C0;
		settings.CANSpeed = 500000;
		settings.TermEnabled = true;
		settings.logLevel = 1;
		for (int x = 0; x < 4; x++) 
		{ 
			settings.tMultiplier[x].adcToVolts = 0.0000625609f;
			settings.tMultiplier[x].A = 1.8794f;
			settings.tMultiplier[x].B = 2.561f;
			settings.tMultiplier[x].C = 17.433f;
			settings.tMultiplier[x].D = 22.679;
			settings.numQuadCells[x] = 0;
			settings.vMultiplier[x] = 0.01285f;
		}
		settings.valid = 0xDE;
		settings.version = CFG_EEPROM_VER;		
		EEPROM.write(0, settings);		
	}

	//do some sanity checks to see if things seem to be set up
	if (settings.currentPackAH == 0 || settings.numQuadCells[0] == 0 ||
		settings.maxPackAH == 0)
	{
		needInitialConfig = true; 
	}
}

void saveEEPROM()
{
	EEPROM.write(0, settings);
	lastWrite = millis();
	SerialUSB.println("Write!");
}

void setupHardware()
{
	loadEEPROM();

	pinModeNonDue(CAN_TERM_1, OUTPUT );  
	pinModeNonDue(CAN_TERM_2, OUTPUT );

	cbHandler = CANBusHandler::getInstance();
	cbHandler->setup();
}

void setup()
{
  Wire.begin(); // wake up I2C bus
  SerialUSB.begin(115200);

  lastWrite = millis() + 2000;

  setupHardware();

  adc = ADCClass::getInstance();
  if (!needInitialConfig) adc->setup();
}

void loop()
{
	if (SerialUSB)
	{
		if (firstConnect)
		{
			firstConnect = false;

			if (!needInitialConfig)
				console.printMenu();			
			else
				console.InitialConfig(); //Locks out the sketch for a while
				adc->setup(); //this was deferred but now must be done.
		}
	}
	if (SerialUSB.available()) 
	{
		console.rcvCharacter((uint8_t)SerialUSB.read());
	}
	adc->loop();
	cbHandler->loop();

	if ((lastWrite + 20000) > millis()) saveEEPROM(); //every 20 seconds
}
