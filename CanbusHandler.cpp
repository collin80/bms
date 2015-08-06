/*
 * CanbusHandler.cpp - Handles setup and control of canbus link
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

#include "CanbusHandler.h"

CANBusHandler *CANBusHandler::instance = NULL;
extern EEPROMSettings settings;
extern STATUS status;
volatile bool DoStatus1 = false;
volatile bool DoStatus2 = false;
volatile bool DoStatus3 = false;
volatile bool DoStatus4 = false;
volatile uint8_t intCounter = 0;

//the status 1 message is sent every 100ms, the others are sent every 500ms
void tickBounce()
{
	DoStatus1 = true;
	intCounter++;
	if (intCounter == 4)
	{
		DoStatus2 = true;
		DoStatus3 = true;
		DoStatus4 = true;
		intCounter = 0;
	}
}

//bounces frame back into class object
void canbusRX(CAN_FRAME *frame)
{
	CANBusHandler::getInstance()->gotFrame(frame);
}

CANBusHandler *CANBusHandler::getInstance()
{
	if (instance == NULL)
	{
		instance = new CANBusHandler();		
	}
	return instance;
}

void CANBusHandler::canbusTermEnable()
{
  digitalWriteNonDue( CAN_TERM_1, HIGH );  
  digitalWriteNonDue( CAN_TERM_2, HIGH );

}

void CANBusHandler::canbusTermDisable()
{
  digitalWriteNonDue( CAN_TERM_1, LOW );  
  digitalWriteNonDue( CAN_TERM_2, LOW );
}

void CANBusHandler::setup()
{
	if (settings.TermEnabled) canbusTermEnable();
	else canbusTermDisable();

	if (settings.CANSpeed < 33333) settings.CANSpeed = 500000; //don't allow stupidly low canbus values
	Can0.begin(settings.CANSpeed, 255); //no enable pin
	if (settings.cab300Address > 0) 
	{
		Can0.watchFor(settings.cab300Address); //allow through only this address for now
		cab300 = new CAB300();
	}
 
	Can0.setGeneralCallback(canbusRX);

	Timer5.attachInterrupt(tickBounce);
	Timer5.start(100000); //100ms

	adc = ADCClass::getInstance();
}

void CANBusHandler::gotFrame(CAN_FRAME *frame)
{
	if (cab300) cab300->processFrame(*frame);

	if (frame->id == (settings.bmsBaseAddress - 0x10)) //BMS control packet
	{
		if (frame->data.byte[0] == 0x10) //reset SOC to 100% externally
		{
			settings.currentPackAH = settings.maxPackAH;
		}
	}
}

void CANBusHandler::loop()
{
	CAN_FRAME frame;
	uint32_t currAH, maxAH, calcAH;
	frame.length = 8;
	if (settings.bmsBaseAddress < 0x7E0) frame.extended = false;
	else frame.extended = true;


	if (DoStatus1)
	{
		DoStatus1 = false;
		frame.id = settings.bmsBaseAddress;	
		BMS_STATUS_1 stat1;
		stat1.packamps = (int16_t)(cab300->getAmps()/10);
		stat1.packvolts = (uint16_t)(adc->getPackVoltage() * 100);		

		//Done this way to avoid overflow issues
		currAH = settings.currentPackAH / 10000;
		maxAH = settings.maxPackAH / 10000;
		calcAH = 255 * currAH;
		calcAH = calcAH / maxAH;
		uint8_t soc = (uint8_t)calcAH;

		stat1.soc = soc;
		stat1.status = status;
		frame.data.value = stat1.value;
		Can0.sendFrame(frame);
	}
	if (DoStatus2)
	{
		DoStatus2 = false;
		frame.id = settings.bmsBaseAddress + 1;
		BMS_STATUS_2 stat2;
		stat2.quad1 = (uint16_t)(adc->getVoltage(0) * 100);
		stat2.quad2 = (uint16_t)(adc->getVoltage(1) * 100);
		stat2.quad3 = (uint16_t)(adc->getVoltage(2) * 100);
		stat2.quad4 = (uint16_t)(adc->getVoltage(3) * 100);
		frame.data.value = stat2.value;
		Can0.sendFrame(frame);
	}

	if (DoStatus3)
	{
		DoStatus3 = false;
		frame.id = settings.bmsBaseAddress + 2;
		BMS_STATUS_3 stat3;
		stat3.quad1 = (uint16_t)(adc->getCellAvgVoltage(0) * 1000);
		stat3.quad2 = (uint16_t)(adc->getCellAvgVoltage(1) * 1000);
		stat3.quad3 = (uint16_t)(adc->getCellAvgVoltage(2) * 1000);
		stat3.quad4 = (uint16_t)(adc->getCellAvgVoltage(3) * 1000);
		frame.data.value = stat3.value;
		Can0.sendFrame(frame);
	}

	if (DoStatus4)
	{
		DoStatus4 = false;
		frame.id = settings.bmsBaseAddress + 3;
		BMS_STATUS_4 stat4;
		stat4.quad1 = (int16_t)(adc->getTemperature(0) * 10);
		stat4.quad2 = (int16_t)(adc->getTemperature(1) * 10);
		stat4.quad3 = (int16_t)(adc->getTemperature(2) * 10);
		stat4.quad4 = (int16_t)(adc->getTemperature(3) * 10);
		frame.data.value = stat4.value;
		Can0.sendFrame(frame);
	}
}
