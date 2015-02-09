/*
 * adc.h - Handles reading the various voltages and temperatures
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

#include <Arduino.h>
#include "SamNonDuePin.h" //Non-supported SAM3X pin library
#include <due_wire.h>
#include <Wire_EEPROM.h>
#include <DueTimer.h>
#include "Logger.h"
#include "config.h"


#ifndef ADCCLASS_H_
#define ADCCLASS_H_

#define SAMPLES	8 //how many samples to use while smoothing

extern volatile bool doADC;

class ADCClass 
{
public:
	void setup();
	static ADCClass* getInstance();
	void handleTick();
	int getRawV(int which);
	int getRawT(int which);
	float getVoltage(int which);
	float getPackVoltage();
	float getTemperature(int which);
	void loop();

private:
	//There are three full readings per second so 32 entries is about 10 seconds worth of data
	//Thus, the average of all these readings is a 10 second average of the pack performance
	int16_t vReading[4][SAMPLES];
	int16_t tReading[4][SAMPLES];
	int vAccum[4];
	int tAccum[4];
	byte vReadingPos, tReadingPos;
	static ADCClass *instance;	

	void setAllVOff();
	void setVEnable(uint8_t which);
	void setAllThermOff();
	void setThermActive(uint8_t which);
	void adsStartConversion(uint8_t addr);
	bool adsGetData(byte addr, int16_t &value);	
};

#endif