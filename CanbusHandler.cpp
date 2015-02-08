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
volatile bool DoStatus1 = false;
volatile bool DoStatus2 = false;
volatile bool DoStatus3 = false;
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
}

void CANBusHandler::gotFrame(CAN_FRAME *frame)
{
	if (cab300) cab300->processFrame(*frame);
}

void CANBusHandler::loop()
{
	if (DoStatus1)
	{
		DoStatus1 = false;
	}
	if (DoStatus2)
	{
		DoStatus2 = false;
	}
	if (DoStatus3)
	{
		DoStatus3 = false;
	}
}
