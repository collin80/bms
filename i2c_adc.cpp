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

	//Thermistor inputs
	pinModeNonDue(SWITCH_THERM1, OUTPUT );
	pinModeNonDue(SWITCH_THERM2, OUTPUT );
	pinModeNonDue(SWITCH_THERM3, OUTPUT );
	pinModeNonDue(SWITCH_THERM4, OUTPUT );
	setAllThermOff();

	Timer3.attachInterrupt(adcTickBounce);
	Timer3.start(80000); //trigger every 80ms
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

//periodic tick that handles reading ADCs and doing other stuff that happens on a schedule.
//There are four voltage readings and four thermistor readings and they are on separate
//ads chips so each can be read every cycle which means it takes exactly 320ms to read the whole pack
//Or, the pack is fully characterized three times per second. Not too shabby!
void ADCClass::handleTick()
{
	static byte vNum, tNum; //which voltage and thermistor reading we're on

	int16_t readValue;

	int x;
	
	//read the values from the previous cycle
	//if there is a problem we won't update the values stored
	if (adsGetData(VIN_ADDR, readValue)) 
	{
		vReading[vNum][vReadingPos] = readValue;
		vReadingPos = (vReadingPos + 1) & (SAMPLES-1);
		vAccum[vNum] = 0;
		for (x = 0; x < SAMPLES; x++)
		{
			vAccum[vNum] += vReading[vNum][x];
		}
		vAccum[vNum] /= SAMPLES;
	}

	if (adsGetData(THERM_ADDR, readValue)) 
	{
		tReading[tNum][tReadingPos] = readValue;	
		tReadingPos = (tReadingPos + 1) & (SAMPLES-1);
		tAccum[tNum] = 0;
		for (x = 0; x < SAMPLES; x++)
		{
			tAccum[tNum] += tReading[tNum][x];
		}
		tAccum[tNum] /= SAMPLES;

	}

	//Set up for the next set of readings
	setVEnable(vNum++);
	adsStartConversion(VIN_ADDR);
	setThermActive(SWITCH_THERM1 + tNum++);
	adsStartConversion(THERM_ADDR);
	
	//constrain these back to valid range 0-3	
	tNum &= 3;

	int16_t vHigh = -10000, vLow = 30000;		
	if (vNum == 4)
	{	
		vNum = 0;
		for (int y = 0; y < 4; y++) 
		{	
			if (vAccum[y] > vHigh) vHigh = vAccum[y];
			if (vAccum[y] < vLow) vLow = vAccum[y];
		}
	
		//Now, if the difference between vLow and vHigh is too high then there is a serious pack balancing problem
		//and we should let someone know
		if (vHigh - vLow > settings.balanceThreshold)
		{
			Logger::debug("Pack voltage imbalance!");
		}
	}

	Logger::debug("V1: %f V2: %f V3: %f V4: %f", getVoltage(0), getVoltage(1), getVoltage(2), getVoltage(3));
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

//note that it reverses the order of inputs. For some reason it seems like everything is backwards but I'm loathe to
//switch up the switching order because it seems to work. But, maybe change the switching pins at some point
//or figure out why things seem backward.
float ADCClass::getVoltage(int which)
{
	if (which < 0) return 0.0f;
	if (which > 3) return 0.0f;
	return (vAccum[3 - which] * settings.vMultiplier[3 - which]);
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
	float y = ((x * x * x) * settings.tMultiplier[which].A) + ((x * x) * settings.tMultiplier[which].B) + (x * settings.tMultiplier[which].C) + settings.tMultiplier[which].D;	
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

