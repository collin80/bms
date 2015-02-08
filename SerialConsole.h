/*
 * SerialConsole.h
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#ifndef SERIALCONSOLE_H_
#define SERIALCONSOLE_H_

#include "config.h"
#include "Logger.h"
#include "due_can.h"
#include "due_wire.h"
#include "Wire_EEPROM.h"
#include "i2c_adc.h"
#include "CanbusHandler.h"

class SerialConsole {
public:
    SerialConsole();
	void printMenu();
	void InitialConfig();
	bool rcvCharacter(uint8_t chr);

protected:
	enum CONSOLE_STATE
	{
		STATE_ROOT_MENU, 
		STATE_GET_CANBUSRATE,
		STATE_GET_CANTERM,
		STATE_GET_CAB300,
		STATE_GET_BALANCE,
		STATE_GET_LOWV,
		STATE_GET_HIGHV,
		STATE_GET_LOWT,
		STATE_GET_HIGHT,
		STATE_GET_QUAD1,
		STATE_GET_QUAD2,
		STATE_GET_QUAD3,
		STATE_GET_QUAD4,
		STATE_GET_MAXAH,
		STATE_GET_CURRAH
	};

private:
	char cmdBuffer[80];
	int ptrBuffer;
	int state;
    
    void init();
	void handleConsoleCmd();
	void handleShortCmd();
    void handleConfigCmd();
	void vCalibrate();
	void getReply();
	void appendCmd(String cmd);
};

#endif /* SERIALCONSOLE_H_ */
