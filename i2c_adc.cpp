/*
 * adc.cpp - Handles reading the various voltages and temperatures
 *
 Copyright (c) 2015 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#include "i2c_adc.h"

const uint8_t VBat[4][2] = {
								{SWITCH_VBAT1_H, SWITCH_VBAT2_L},{SWITCH_VBAT2_H, SWITCH_VBAT3_L}, 
								{SWITCH_VBAT3_H, SWITCH_VBAT4_L},{SWITCH_VBAT4_H,SWITCH_VBATRTN}
						   };

extern EEPROMSettings settings;
extern STATUS status;

ADCClass* ADCClass::instance = NULL;
volatile bool doADC = false;

void adcTickBounce()
{
	doADC = true;	
}

ADCClass* ADCClass::getInstance()
{
	if (instance == NULL)
	{
		instance = new ADCClass();		
	}
	return instance;
}

void ADCClass::setup()
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
	setVEnable(0);

	//Thermistor inputs
	pinModeNonDue(SWITCH_THERM1, OUTPUT );
	pinModeNonDue(SWITCH_THERM2, OUTPUT );
	pinModeNonDue(SWITCH_THERM3, OUTPUT );
	pinModeNonDue(SWITCH_THERM4, OUTPUT );
	setAllThermOff();
	setThermActive(SWITCH_THERM1);

	Timer3.attachInterrupt(adcTickBounce);
	Timer3.start(125000); //trigger every 125ms
}


void ADCClass::setAllVOff()
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

void ADCClass::setVEnable(uint8_t which)
{
	if (which > 3) return;
	setAllVOff();
	digitalWriteNonDue(VBat[which][0], HIGH);
	digitalWriteNonDue(VBat[which][1], HIGH);
}

void ADCClass::setAllThermOff()
{
  digitalWriteNonDue(SWITCH_THERM1, LOW );
  digitalWriteNonDue(SWITCH_THERM2, LOW);
  digitalWriteNonDue(SWITCH_THERM3, LOW );
  digitalWriteNonDue(SWITCH_THERM4, LOW );
}

void ADCClass::setThermActive(uint8_t which)
{
	if (which < SWITCH_THERM1) return;
	if (which > SWITCH_THERM4) return;
	setAllThermOff();
	digitalWriteNonDue(which, HIGH );
}

//ask for a reading to start. Currently we support 15 reads per second so one must wait
// at least 1000 / 15 = 67ms before trying to get the answer
void ADCClass::adsStartConversion(uint8_t addr)
{
  Wire.beginTransmission(addr); 
  Wire.write(ADS_CONFIG);
  Wire.endTransmission();
}

//returns true if data was waiting or false if it was not
bool ADCClass::adsGetData(byte addr, int16_t &value)
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

/*
  periodic tick that handles reading ADCs and doing other stuff that happens on a schedule.
  There are four voltage readings and four thermistor readings and they are on separate
  ads chips. What happens is a new source is selected and then a conversion is started. Later
  on we read back the value. The code runs like this:
  1. Start conversion for voltage
  2. Read that conversion and select a new voltage source
  3. Start conversion for temperature
  4. Read that conversion and select a new temperature source
  5. Calculate all fault types

  This way there is always a big delay between selecting a source, starting the conversion, and reading
  the new value.
*/
void ADCClass::handleTick()
{
	//these track which reading we are about to request
	static byte vNum = 0, tNum = 0; //which voltage and thermistor reading we're on
	static byte Operation = 0; //quad way operation 0 = read voltage, 1 = start new voltage reading, 2 = read temperature, 3 = start new temperature reading and calculate faults
	static uint16_t EEPROMWriteCounter = 0;

	int32_t vTemp, tTemp;

	int16_t readValue;

	int x;
	float divisor;

	float vHigh, vLow, quadVolt, perVolt;
	int perMilliVolt;
	int thisTemperature;

	//This is just an opportune place because it is the only place that currently has a timer set up. Probably should move
	//this to a better place and set up another timer
	//125ms tick * 500 = 62.5 second interval for writing which gives us at least a full year of run time.
	EEPROMWriteCounter++;
	if (EEPROMWriteCounter > 500)
	{
		EEPROMWriteCounter = 0;
		EEPROM.write(0, settings);
	}

	switch (Operation) 
	{
	case 0: //start by asking to begin an ADC reading for voltage
		adsStartConversion(VIN_ADDR);
		break;
	case 1: //its been some time since we asked for the reading so ask for it now
		//read the values from the previous cycle
		//if there is a problem we won't update the values stored
		if (adsGetData(VIN_ADDR, readValue))
		{
			vReading[vNum][vReadingPos] = readValue;
			vReadingPos = (vReadingPos + 1) % SAMPLES;
			vTemp = 0;
			for (x = 0; x < SAMPLES; x++)
			{
				vTemp += vReading[vNum][x];
			}
			vTemp /= SAMPLES;
			vAccum[vNum] = vTemp;
		}
		else Logger::error("Error reading voltage");

		divisor = 1.0f;
		if (settings.numQuadCells[vNum] > 0) divisor = (float)settings.numQuadCells[vNum];
		Logger::debug("V%i: %f AV%i %f", vNum, getVoltage(vNum), vNum, getVoltage(vNum) / divisor);		
		//if (vNum == 3) Logger::debug("Total system voltage: %f", getVoltage(0) + getVoltage(1) + getVoltage(2) + getVoltage(3));

		vNum = (vNum + 1) & 3;

		setVEnable(vNum);
		break;

	case 2:
		adsStartConversion(THERM_ADDR);
		break;

	case 3:
		if (adsGetData(THERM_ADDR, readValue))
		{
			//Logger::debug("TL: %i", readValue);
			tReading[tNum][tReadingPos] = readValue;
			tReadingPos = (tReadingPos + 1) % SAMPLES;
			tTemp = 0;
			for (x = 0; x < SAMPLES; x++)
			{
				tTemp += tReading[tNum][x];
			}
			tTemp /= SAMPLES;
			tAccum[tNum] = tTemp;
		}
		else Logger::error("Error reading temperature");

		Logger::debug("T%i: %f", tNum, getTemperature(tNum));
		Logger::debug(" ");

		tNum = tNum + 1;
		tNum &= 3;
		setThermActive(SWITCH_THERM1 + tNum);
		break;
	case 4: //calculate the various faults
		vHigh = -10000.0f;
		vLow = 30000.0f;

		//default to being OK and set them off if necessary.
		status.CHARGE_OK = 1;
		status.DISCHARGE_OK = 1;
		status.LOWV = 0;
		status.LOWT = 0;
		status.HIGHT = 0;
		status.HIGHV = 0;

		for (int y = 0; y < 4; y++)
		{
			quadVolt = getVoltage(y);
			divisor = 1.0f;
			if (settings.numQuadCells[y] > 0) divisor = (float)settings.numQuadCells[y];
			perVolt = quadVolt / divisor;
			perMilliVolt = (int)(quadVolt * 1000);
			thisTemperature = (int)(getTemperature(y) * 10);
			if (perVolt > vHigh) vHigh = perVolt;
			if (perVolt < vLow) vLow = perVolt;

			if (perMilliVolt > settings.highThreshold)
			{
				status.HIGHV = 1;
				status.CHARGE_OK = 0;
			}

			if (perMilliVolt < settings.lowThreshold)
			{
				status.LOWV = 1;
				status.DISCHARGE_OK = 0;
			}

			if (thisTemperature > settings.highTempThresh)
			{
				status.HIGHT = 1;
				status.DISCHARGE_OK = 0;
				status.CHARGE_OK = 0;
			}

			if (thisTemperature < settings.lowTempThresh)
			{
				status.LOWT = 1;
				status.DISCHARGE_OK = 0;
				status.CHARGE_OK = 0;
			}
		}

		//Now, if the difference between vLow and vHigh is too high then there is a serious pack balancing problem
		//and we should let someone know
		if ((int)((vHigh - vLow) * 1000) > settings.balanceThreshold)
		{
			Logger::debug("Pack voltage imbalance!");
			status.IMBALANCE = 1;
			status.CHARGE_OK = 0; //not OK to charge if pack is out of wack
			status.DISCHARGE_OK = 0; //also not OK to discharge
		}
		else status.IMBALANCE = 0;
		break;
	}
	
	Operation++;
	if (Operation == 5) Operation = 0;
}

int ADCClass::getRawV(int which)
{
	if (which < 0) return 0;
	if (which > 3) return 0;
	return vAccum[which];
}

int ADCClass::getRawT(int which)
{
	if (which < 0) return 0;
	if (which > 3) return 0;
	return tAccum[which];
}

float ADCClass::getVoltage(int which)
{
	if (which < 0) return 0.0f;
	if (which > 3) return 0.0f;
	return (vAccum[which] * settings.vMultiplier[which]);
}

float ADCClass::getCellAvgVoltage(int which)
{
	float divisor = 1.0f;
	if (which < 0) return 0.0f;
	if (which > 3) return 0.0f;
	if (settings.numQuadCells[which] > 0) divisor = (float)settings.numQuadCells[which];
	return (vAccum[which] * settings.vMultiplier[which] / divisor);
}

float ADCClass::getPackVoltage()
{
	float accum = 0.0f;
	for (int x = 0; x < 4; x++) accum += getVoltage(x);
	return accum;
}

float ADCClass::getTemperature(int which)
{
	if (which < 0) return 0.0f;
	if (which > 3) return 0.0f;

	//temperature is much more complicated to calculate. Luckily we only do it when asked
	//Basically just use a third order polynomial. The results are actually fairly linear but off by enough
	//that it seems best to use a third order equation to get the best accuracy / time trade off.
	//It isn't strictly necessary to have an ADC to Volts multiplier. The A,B,C coefficients could have accommodated this
	//but then they'd be really, really tiny values and that's asking for trouble with floating point
	//that is - trying to multiply very large values against very small - It's a bad idea.
	float x = tAccum[which] * settings.tMultiplier[which].adcToVolts;
	//done in stages to minimize the # of float multiplications to do
	float y = settings.tMultiplier[which].D;
	y += (x * settings.tMultiplier[which].C);
	x *= x; //now x = x^2
	y += (x * settings.tMultiplier[which].B);
	x *= x; //now x = x^3
	y += (x * settings.tMultiplier[which].A);
	return (y);
}

void ADCClass::loop()
{
	if (doADC)
	{
		ADCClass::getInstance()->handleTick();
		doADC = false;
	}
}

