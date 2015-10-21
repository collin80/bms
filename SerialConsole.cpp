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

//doin' it old school here... should be doing this object oriented but instead this is 
//the cheap and dirty approach
extern EEPROMSettings settings;

extern ADCClass* adc;
extern CANBusHandler *cbHandler;

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
void SerialConsole::vCalibrate()
{
	delay(1000); //just be sure 
	float oldV, newV, scale;

	for (int subpack = 1; subpack < 5; subpack++)
	{
		oldV = adc->getVoltage(subpack - 1);
		SerialUSB.print("Reported voltage of subpack ");
		SerialUSB.print(subpack);
		SerialUSB.print(": ");
		SerialUSB.println(oldV);		
		SerialUSB.print("Enter measured voltage: ");
		String input = SerialUSB.readString();
		newV = atof((char *)input.c_str());
		scale = newV / oldV;
		settings.vMultiplier[subpack - 1] *= scale;
		SerialUSB.println();
		delay(1000); //just be sure 
	}
	Serial.println("Voltages have been calibrated and calibration saved to EEPROM");
	EEPROM.write(0, settings);
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
	SerialUSB.println("R = reset to factory defaults");
	SerialUSB.println();
	SerialUSB.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
    SerialUSB.println();

    Logger::console("LOGLEVEL=%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", settings.logLevel);
	SerialUSB.println();

	Logger::console("TERMEN=%i - Enable/Disable CAN Termination (0 = Disable, 1 = Enable)", settings.TermEnabled);
	Logger::console("CANSPEED=%i - Set speed of CAN in baud (125000, 250000, etc)", settings.CANSpeed);
	SerialUSB.println();

	Logger::console("CABADDR=%x - Set address of CAB300 sensor", settings.cab300Address);
	SerialUSB.println();

	Logger::console("BASEADDR=%x - Set base address for status messages", settings.bmsBaseAddress);
	SerialUSB.println();

	Logger::console("BALTHR=%i - Set balancing threshold (millivolts)", settings.balanceThreshold);
	Logger::console("LOWTHR=%i - Set low cell voltage threshold (millivolts)", settings.lowThreshold);
	Logger::console("HIGHTHR=%i - Set high cell voltage (millivolts)", settings.highThreshold);
	Logger::console("LOWTEMP=%i - Set lowest acceptable temperature (In tenths of deg C)", settings.lowTempThresh);
	Logger::console("HIGHTEMP=%i - Set highest acceptable temperature (In tenths of a deg C)", settings.highTempThresh);
	SerialUSB.println();

	Logger::console("Q1CELLS=%i - Set number of series cells in quadrant 1", settings.numQuadCells[0]);
	Logger::console("Q2CELLS=%i - Set number of series cells in quadrant 2", settings.numQuadCells[1]);
	Logger::console("Q3CELLS=%i - Set number of series cells in quadrant 3", settings.numQuadCells[2]);
	Logger::console("Q4CELLS=%i - Set number of series cells in quadrant 4", settings.numQuadCells[3]);
	SerialUSB.println();

	Logger::console("MAXAH=%i - Set pack AH capacity (in tenths of an AH)", (settings.maxPackAH / 1000000));
	Logger::console("CURRAH=%i - Set current AH state of pack (tenths of AH)", (settings.currentPackAH / 1000000));
	SerialUSB.println();

	Logger::console("VMULT1=%f - Set voltage multiplier for bank 1", settings.vMultiplier[0]);
	Logger::console("VMULT2=%f - Set voltage multiplier for bank 2", settings.vMultiplier[1]);
	Logger::console("VMULT3=%f - Set voltage multiplier for bank 3", settings.vMultiplier[2]);
	Logger::console("VMULT4=%f - Set voltage multiplier for bank 4", settings.vMultiplier[3]);
	SerialUSB.println();
	
	Logger::console("TMULT1A=%f - Set temperature coefficient A for bank 1", settings.tMultiplier[0].A);
	Logger::console("TMULT1B=%f - Set temperature coefficient B for bank 1", settings.tMultiplier[0].B);
	Logger::console("TMULT1C=%f - Set temperature coefficient C for bank 1", settings.tMultiplier[0].C);
	Logger::console("TMULT1D=%f - Set temperature conversion factor for bank 1", settings.tMultiplier[0].adcToVolts);
	SerialUSB.println();

	Logger::console("TMULT2A=%f - Set temperature coefficient A for bank 2", settings.tMultiplier[1].A);
	Logger::console("TMULT2B=%f - Set temperature coefficient B for bank 2", settings.tMultiplier[1].B);
	Logger::console("TMULT2C=%f - Set temperature coefficient C for bank 2", settings.tMultiplier[1].C);
	Logger::console("TMULT2D=%f - Set temperature conversion factor for bank 2", settings.tMultiplier[1].adcToVolts);
	SerialUSB.println();

	Logger::console("TMULT3A=%f - Set temperature coefficient A for bank 3", settings.tMultiplier[2].A);
	Logger::console("TMULT3B=%f - Set temperature coefficient B for bank 3", settings.tMultiplier[2].B);
	Logger::console("TMULT3C=%f - Set temperature coefficient C for bank 3", settings.tMultiplier[2].C);
	Logger::console("TMULT3D=%f - Set temperature conversion factor for bank 3", settings.tMultiplier[2].adcToVolts);
	SerialUSB.println();

	Logger::console("TMULT4A=%f - Set temperature coefficient A for bank 4", settings.tMultiplier[3].A);
	Logger::console("TMULT4B=%f - Set temperature coefficient B for bank 4", settings.tMultiplier[3].B);
	Logger::console("TMULT4C=%f - Set temperature coefficient C for bank 4", settings.tMultiplier[3].C);
	Logger::console("TMULT4D=%f - Set temperature conversion factor for bank 4", settings.tMultiplier[3].adcToVolts);
	SerialUSB.println();

}

/*	There is a help menu (press H or h or ?)
 This is no longer going to be a simple single character console.
 Now the system can handle up to 80 input characters. Commands are submitted
 by sending line ending (LF, CR, or both)
 */
bool SerialConsole::rcvCharacter(uint8_t chr) {
	if (chr == 10 || chr == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
		return true;
	} else {
		cmdBuffer[ptrBuffer++] = (unsigned char) chr;
		if (ptrBuffer > 79)
			ptrBuffer = 79;
	}
	return false;
}

void SerialConsole::appendCmd(String cmd)
{
	String buildString;
	const char* tempStr;

	buildString = cmd;
	buildString += String(cmdBuffer);		
	tempStr = buildString.c_str();
	ptrBuffer = strlen(tempStr);
	for (int x = 0; x < ptrBuffer; x++)
	{
		cmdBuffer[x] = tempStr[x];
	}
	cmdBuffer[ptrBuffer] = 0;
}

void SerialConsole::handleConsoleCmd() {

	if (!adc) adc = ADCClass::getInstance();
	if (!cbHandler) cbHandler = CANBusHandler::getInstance();

	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate

	switch (state)
	{
	case STATE_ROOT_MENU:
		if (ptrBuffer == 1) { //command is a single ascii character
			handleShortCmd();
		} else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}		
		break;
	case STATE_GET_CANBUSRATE:
		appendCmd("CANSPEED=");
		handleConfigCmd();
		break;
	case STATE_GET_CANTERM:
		appendCmd("TERMEN=");
		handleConfigCmd();
		break;
	case STATE_GET_CAB300:
		appendCmd("CABADDR=");
		handleConfigCmd();
		break;
	case STATE_GET_BALANCE:
		appendCmd("BALTHR=");
		handleConfigCmd();
		break;
	case STATE_GET_LOWV:
		appendCmd("LOWTHR=");
		handleConfigCmd();
		break;
	case STATE_GET_HIGHV:
		appendCmd("HIGHTHR=");
		handleConfigCmd();
		break;
	case STATE_GET_LOWT:
		appendCmd("LOWTEMP=");
		handleConfigCmd();
		break;
	case STATE_GET_HIGHT:
		appendCmd("HIGHTEMP=");
		handleConfigCmd();
		break;
	case STATE_GET_QUAD1:
		appendCmd("Q1CELLS=");
		handleConfigCmd();
		break;
	case STATE_GET_QUAD2:
		appendCmd("Q2CELLS=");
		handleConfigCmd();
		break;
	case STATE_GET_QUAD3:
		appendCmd("Q3CELLS=");
		handleConfigCmd();
		break;
	case STATE_GET_QUAD4:
		appendCmd("Q4CELLS=");
		handleConfigCmd();
		break;
	case STATE_GET_MAXAH:
		appendCmd("MAXAH=");
		handleConfigCmd();
		break;
	case STATE_GET_CURRAH:
		appendCmd("CURRAH=");
		handleConfigCmd();
		break;

	}

	ptrBuffer = 0; //reset line counter once the line has been processed
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

	if (cmdString == String("TERMEN")) {
		if (newValue < 0) newValue = 0;
		if (newValue > 1) newValue = 1;
		Logger::console("Setting CAN Termination Enable to %i", newValue);
		settings.TermEnabled = newValue;
		if (newValue == 1) cbHandler->canbusTermEnable();
		else cbHandler->canbusTermDisable();
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
			Logger::console("Setting CAB Address to %x", newValue);
			settings.cab300Address = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid address! Enter a value 0 - 0x7FF");
	} else if (cmdString == String("BASEADDR")) {
		if (newValue > 0 && newValue <= 0x1FFFFFFF) 
		{
			Logger::console("Setting base Address to %x", newValue);
			settings.bmsBaseAddress = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid address! Enter a value 0 - 0x1FFFFFFF");
	} else if (cmdString == String("BALTHR")) {
		if (newValue > 0) 
		{
			Logger::console("Setting balancing threshold to %i", newValue);
			settings.balanceThreshold = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! It has to be positive!");
	} else if (cmdString == String("LOWTHR")) {
		if (newValue > 0) 
		{
			Logger::console("Setting low voltage threshold to %i", newValue);
			settings.lowThreshold = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! It has to be positive!");
	} else if (cmdString == String("HIGHTHR")) {
		if (newValue > 0) 
		{
			Logger::console("Setting high voltage threshold to %i", newValue);
			settings.highThreshold = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! It has to be positive!");
	} else if (cmdString == String("LOWTEMP")) {
		if (newValue >= -400  && newValue <= 1000) 
		{
			Logger::console("Setting low temperature threshold to %i", newValue);
			settings.lowTempThresh = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! Set between -400 and 1000");
	} else if (cmdString == String("HIGHTEMP")) {
		if (newValue >= -400 && newValue <= 1000) 
		{
			Logger::console("Setting high temperature threshold to %i", newValue);
			settings.highTempThresh = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid threshold! Set between -400 and 1000");
	} else if (cmdString == String("Q1CELLS")) {
		if (newValue >= 0 && newValue <= 120) 
		{
			Logger::console("Setting number of cells in quad 1 to %i", newValue);
			settings.numQuadCells[0] = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Must enter value between 0 and 120");
	} else if (cmdString == String("Q2CELLS")) {
		if (newValue >= 0 && newValue <= 120) 
		{
			Logger::console("Setting number of cells in quad 1 to %i", newValue);
			settings.numQuadCells[1] = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Must enter value between 0 and 120");
	} else if (cmdString == String("Q3CELLS")) {
		if (newValue >= 0 && newValue <= 120) 
		{
			Logger::console("Setting number of cells in quad 1 to %i", newValue);
			settings.numQuadCells[2] = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Must enter value between 0 and 120");
	} else if (cmdString == String("Q4CELLS")) {
		if (newValue >= 0 && newValue <= 120) 
		{
			Logger::console("Setting number of cells in quad 1 to %i", newValue);
			settings.numQuadCells[3] = newValue;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Must enter value between 0 and 120");
	} else if (cmdString == String("MAXAH")) {
		if (newValue > 0) 
		{
			Logger::console("Setting pack AH capacity to to %i", newValue);
			settings.maxPackAH = newValue * 1000000;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Entered value must be positive!");
	} else if (cmdString == String("CURRAH")) {
		if (newValue > 0) 
		{
			Logger::console("Setting pack AH capacity to to %i", newValue);
			settings.currentPackAH = newValue * 1000000;
			writeEEPROM = true;
		}
		else Logger::console("Invalid! Entered value must be positive!");
	} else if (cmdString == String("VMULT1")) {
		Logger::console("Setting voltage multiplier bank 1 to %f", newValFloat);
		settings.vMultiplier[0] = newValFloat;
		//writeEEPROM = true;
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
	} else if (cmdString == String("TMULT1A")) {
		Logger::console("Setting temperature coefficient A bank 1 to %f", newValFloat);
		settings.tMultiplier[0].A = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT2A")) {
		Logger::console("Setting temperature coefficient A bank 2 to %f", newValFloat);
		settings.tMultiplier[1].A = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT3A")) {
		Logger::console("Setting temperature coefficient A bank 3 to %f", newValFloat);
		settings.tMultiplier[2].A = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT4A")) {
		Logger::console("Setting temperature coefficient A bank 4 to %f", newValFloat);
		settings.tMultiplier[3].A = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT1B")) {
		Logger::console("Setting temperature coefficient B bank 1 to %f", newValFloat);
		settings.tMultiplier[0].B = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT2B")) {
		Logger::console("Setting temperature coefficient B bank 2 to %f", newValFloat);
		settings.tMultiplier[1].B = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT3B")) {
		Logger::console("Setting temperature coefficient B bank 3 to %f", newValFloat);
		settings.tMultiplier[2].B = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT4B")) {
		Logger::console("Setting temperature coefficient B bank 4 to %f", newValFloat);
		settings.tMultiplier[3].B = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT1C")) {
		Logger::console("Setting temperature coefficient C bank 1 to %f", newValFloat);
		settings.tMultiplier[0].C = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT2C")) {
		Logger::console("Setting temperature coefficient C bank 2 to %f", newValFloat);
		settings.tMultiplier[1].C = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT3C")) {
		Logger::console("Setting temperature coefficient C bank 3 to %f", newValFloat);
		settings.tMultiplier[2].C = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT4C")) {
		Logger::console("Setting temperature coefficient C bank 4 to %f", newValFloat);
		settings.tMultiplier[3].C = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT1D")) {
		Logger::console("Setting temperature conversion factor bank 1 to %f", newValFloat);
		settings.tMultiplier[0].D = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT2D")) {
		Logger::console("Setting temperature conversion factor bank 2 to %f", newValFloat);
		settings.tMultiplier[1].D = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT3D")) {
		Logger::console("Setting temperature conversion factor bank 3 to %f", newValFloat);
		settings.tMultiplier[2].D = newValFloat;
		writeEEPROM = true;
	} else if (cmdString == String("TMULT4D")) {
		Logger::console("Setting temperature conversion factor bank 4 to %f", newValFloat);
		settings.tMultiplier[3].D = newValFloat;
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


	}
}

void SerialConsole::getReply()
{
	for (;;)
	{
		if (SerialUSB.available()) 
		{
			if (rcvCharacter((uint8_t)SerialUSB.read())) return;
		}
	}
}

/*
Creates a nice interface to automatically ask for all the stuff that must be set up
upon a new installation. Actually calls into the existing routines so it's really just
an easier approach to the existing routines
*/
void SerialConsole::InitialConfig()
{
	SerialUSB.println();
	SerialUSB.println();
	SerialUSB.println();
	SerialUSB.println("                            Initial Setup");
	SerialUSB.println();
	SerialUSB.println("It appears that this is a new BMS installation. Let's get you set up.");
	SerialUSB.println("Enter 0 for anything you don't know the answer to");
	SerialUSB.println();
	
	state = STATE_GET_CANBUSRATE;
	SerialUSB.println("1. What canbus baud rate do you need?");
	getReply();

	state = STATE_GET_CANTERM;
	SerialUSB.println("2. Do you need canbus termination at this BMS? (0 = no, 1 = yes)");
	getReply();
	
	state = STATE_GET_CAB300;
	SerialUSB.println("3. If you have a CAB300 sensor installed please enter its address (0 = no CAB300)");
	getReply();
	
	state = STATE_GET_BALANCE;
	SerialUSB.println("4. Maximum variation between average cell voltage to allow? (in millivolts) ");
	getReply();
	
	state = STATE_GET_LOWV;
	SerialUSB.println("5. Lowest average cell voltage to allow? (in millivolts) ");
	getReply();

	state = STATE_GET_HIGHV;
	SerialUSB.println("6. Highest average cell voltage to allow? (in millivolts) ");
	getReply();
	
	state = STATE_GET_LOWT;
	SerialUSB.println("7. Lowest cell temperature to allow? (Tenths of a degree C) ");
	getReply();

	state = STATE_GET_HIGHT;
	SerialUSB.println("8. Highest cell temperature to allow? (Tenths of a degree C) ");
	getReply();
    
	state = STATE_GET_QUAD1;
	SerialUSB.println("Now, the pack should be divided into four quadrants.");
	SerialUSB.println("9. Number of series cells in first quadrant? ");
	getReply();
	
	state = STATE_GET_QUAD2;
	SerialUSB.println("10. Number of series cells in second quadrant? ");
	getReply();
	
	state = STATE_GET_QUAD3;
	SerialUSB.println("11. Number of series cells in third quadrant? ");
	getReply();
	
	state = STATE_GET_QUAD4;
	SerialUSB.println("12. Number of series cells in fourth quadrant? ");
	getReply();
	
	state = STATE_GET_MAXAH;
	SerialUSB.println("13. How many amp hours is this pack (in tenths of an AH)");
	getReply();
	
	state = STATE_GET_CURRAH;
	SerialUSB.println("14. How many amp hours would you estimate the pack is charged to? (Tenths of an AH) ");
	getReply();
	
	SerialUSB.println();
	SerialUSB.println();
	SerialUSB.println("That's it! You're all set! I'm going to enable the BMS now!");
	state = STATE_ROOT_MENU;
}