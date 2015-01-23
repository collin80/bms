/*
 * SerialConsole.cpp
 *
 Copyright (c) 2014 Collin Kidder

 Shamelessly copied from the version in GEVCU

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

#include "SerialConsole.h"

extern EEPROMSettings settings;

SerialConsole::SerialConsole() {
	init();
}

void SerialConsole::init() {
	//State variables for serial console
	ptrBuffer = 0;
	state = STATE_ROOT_MENU;      
}

//For each of the quadrants get the reading and show it to the user. Ask them to input
//their reading and this will correct any issues.
void vCalibrate()
{
	delay(1000); //just be sure 
}

void tCalibrate()
{
}

void SerialConsole::printMenu() {
	char buff[80];
	//Show build # here as well in case people are using the native port and don't get to see the start up messages
	SerialUSB.print("Build number: ");
	SerialUSB.println(CFG_BUILD_NUM);
	SerialUSB.println("System Menu:");
	SerialUSB.println();
	SerialUSB.println("Enable line endings of some sort (LF, CR, CRLF)");
	SerialUSB.println();
	SerialUSB.println("Short Commands:");
	SerialUSB.println("h = help (displays this message)");
	SerialUSB.println("V = Calibrate voltage multipliers");
	SerialUSB.println("T = Calibrate temperature multipliers");
	SerialUSB.println("R = reset to factory defaults");
	SerialUSB.println();
	SerialUSB.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
    SerialUSB.println();

    Logger::console("LOGLEVEL=%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", settings.logLevel);
	SerialUSB.println();

	Logger::console("CANEN=%i - Enable/Disable CAN (0 = Disable, 1 = Enable)", settings.CAN_Enabled);
	Logger::console("CANSPEED=%i - Set speed of CAN in baud (125000, 250000, etc)", settings.CANSpeed);
	SerialUSB.println();

	Logger::console("CABADDR=%i - Set address of CAB300 sensor", settings.cab300Address);
	SerialUSB.println();

	Logger::console("BALTHR=%i - Set balancing threshold (millivolts)", settings.balanceThreshold);
	SerialUSB.println();

	Logger::console("VMULT1=%f - Set voltage multiplier for bank 1", settings.vMultiplier[0]);
	Logger::console("VMULT2=%f - Set voltage multiplier for bank 2", settings.vMultiplier[1]);
	Logger::console("VMULT3=%f - Set voltage multiplier for bank 3", settings.vMultiplier[2]);
	Logger::console("VMULT4=%f - Set voltage multiplier for bank 4", settings.vMultiplier[3]);
	SerialUSB.println();

	Logger::console("TMULT1=%f - Set temperature multiplier for bank 1", settings.tMultiplier[0]);
	Logger::console("TMULT2=%f - Set temperature multiplier for bank 2", settings.tMultiplier[1]);
	Logger::console("TMULT3=%f - Set temperature multiplier for bank 3", settings.tMultiplier[2]);
	Logger::console("TMULT4=%f - Set temperature multiplier for bank 4", settings.tMultiplier[3]);
	SerialUSB.println();
}

/*	There is a help menu (press H or h or ?)
 This is no longer going to be a simple single character console.
 Now the system can handle up to 80 input characters. Commands are submitted
 by sending line ending (LF, CR, or both)
 */
void SerialConsole::rcvCharacter(uint8_t chr) {
	if (chr == 10 || chr == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
	} else {
		cmdBuffer[ptrBuffer++] = (unsigned char) chr;
		if (ptrBuffer > 79)
			ptrBuffer = 79;
	}
}

void SerialConsole::handleConsoleCmd() {
	if (state == STATE_ROOT_MENU) {
		if (ptrBuffer == 1) { //command is a single ascii character
			handleShortCmd();
		} else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}
		ptrBuffer = 0; //reset line counter once the line has been processed
	}
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
 comparison purposes.
 */
void SerialConsole::handleConfigCmd() {
	int i;
	int newValue;
	float newValFloat;
	char *newString;
	bool writeEEPROM = false;

	//Logger::debug("Cmd size: %i", ptrBuffer);
	if (ptrBuffer < 6)
		return; //4 digit command, =, value is at least 6 characters
	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
	String cmdString = String();
	unsigned char whichEntry = '0';
	i = 0;

	while (cmdBuffer[i] != '=' && i < ptrBuffer) {
	 cmdString.concat(String(cmdBuffer[i++]));
	}
	i++; //skip the =
	if (i >= ptrBuffer)
	{
		Logger::console("Command needs a value..ie TORQ=3000");
		Logger::console("");
		return; //or, we could use this to display the parameter instead of setting
	}

	// strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
	newValue = strtol((char *) (cmdBuffer + i), NULL, 0); //try to turn the string into a number
	newValFloat = strtof((char *) (cmdBuffer + i), NULL); //try to turn the string into a FP number
	newString = (char *)(cmdBuffer + i); //leave it as a string

	cmdString.toUpperCase();

	if (cmdString == String("CANEN")) {
		if (newValue < 0) newValue = 0;
		if (newValue > 1) newValue = 1;
		Logger::console("Setting CAN Enabled to %i", newValue);
		settings.CAN_Enabled = newValue;
		if (newValue == 1) Can0.begin(settings.CANSpeed, 255);
		else Can0.disable();
		writeEEPROM = true;
	} else if (cmdString == String("CANSPEED")) {
		if (newValue > 0 && newValue <= 1000000) 
		{
			Logger::console("Setting CAN Baud Rate to %i", newValue);
			settings.CANSpeed = newValue;
			Can0.begin(settings.CANSpeed, 255);
			writeEEPROM = true;
		}
		else Logger::console("Invalid baud rate! Enter a value 1 - 1000000");
	} else if (cmdString == String("CABADDR")) {
		if (newValue > 0 && newValue <= 0x7FF) 
		{
			Logger::console("Setting CAB Address to %i", newValue);
			settings.cab300Address = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid address! Enter a value 0 - 7FF");
	} else if (cmdString == String("BALTHR")) {
		if (newValue > 0) 
		{
			Logger::console("Setting balancing threshold to %i", newValue);
			settings.balanceThreshold = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! It has to be positive!");
	} else if (cmdString == String("VMULT1")) {
		Logger::console("Setting voltage multiplier bank 1 to %f", newValFloat);
		settings.vMultiplier[0] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("VMULT2")) {
		Logger::console("Setting voltage multiplier bank 2 to %f", newValFloat);
		settings.vMultiplier[1] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("VMULT3")) {
		Logger::console("Setting voltage multiplier bank 3 to %f", newValFloat);
		settings.vMultiplier[2] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("VMULT4")) {
		Logger::console("Setting voltage multiplier bank 4 to %f", newValFloat);
		settings.vMultiplier[3] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT1")) {
		Logger::console("Setting temperature multiplier bank 1 to %f", newValFloat);
		settings.tMultiplier[0] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT2")) {
		Logger::console("Setting temperature multiplier bank 2 to %f", newValFloat);
		settings.tMultiplier[1] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT3")) {
		Logger::console("Setting temperature multiplier bank 3 to %f", newValFloat);
		settings.tMultiplier[2] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT4")) {
		Logger::console("Setting temperature multiplier bank 4 to %f", newValFloat);
		settings.tMultiplier[3] = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("LOGLEVEL")) {
		switch (newValue) {
		case 0:
			Logger::setLoglevel(Logger::Debug);
			Logger::console("setting loglevel to 'debug'");
			writeEEPROM = true;
			break;
		case 1:
			Logger::setLoglevel(Logger::Info);
			Logger::console("setting loglevel to 'info'");
			writeEEPROM = true;
			break;
		case 2:
			Logger::console("setting loglevel to 'warning'");
			Logger::setLoglevel(Logger::Warn);
			writeEEPROM = true;
			break;
		case 3:
			Logger::console("setting loglevel to 'error'");
			Logger::setLoglevel(Logger::Error);
			writeEEPROM = true;
			break;
		case 4:
			Logger::console("setting loglevel to 'off'");
			Logger::setLoglevel(Logger::Off);
			writeEEPROM = true;
			break;
		}
	} 
	else {
		Logger::console("Unknown command");
	}
	if (writeEEPROM) 
	{
		EEPROM.write(0, settings);
	}
}

void SerialConsole::handleShortCmd() {
	uint8_t val;

	switch (cmdBuffer[0]) {
	case 'h':
	case '?':
	case 'H':
		printMenu();
		break;
	case 'R': //reset to factory defaults.
		settings.version = 0xFF;
		EEPROM.write(0, settings);
		Logger::console("Power cycle to reset to factory defaults");
		break;
	case 'V':
		//voltage calibration
		vCalibrate();
		break;
	case 'T':
		//tempurature
		tCalibrate();
		break;

	}
}


