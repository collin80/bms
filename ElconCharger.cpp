#include "ElconCharger.h"

extern EEPROMSettings settings;
extern STATUS status;

ElconCharger::ElconCharger()
{
	wantCharging = true;
	outputVoltage = 0;
	outputCurrent = 0;
	statusFlags = 0;
}

/*
For simplicity this code will follow the charger. If the charger sends a status message we'll reply with
a command. This way it is easy to know when the charger is active and when it is not.
*/
void ElconCharger::processFrame(CAN_FRAME &frame)
{
	if (frame.id == 0x18FF50E5)
	{
		Logger::debug("Got frame from elcon charger");
		outputVoltage = (frame.data.bytes[0] * 256) + (frame.data.bytes[1]);
		outputCurrent = (frame.data.bytes[2] * 256) + (frame.data.bytes[3]);
		statusFlags = frame.data.bytes[4] & 0x1f;
		if (statusFlags & 1)
		{
			Logger::error("Elcon charger: Hardware failure!");
		}
		if (statusFlags & 2)
		{
			Logger::error("Elcon charger: too hot!");
		}
		if (statusFlags & 4)
		{
			Logger::error("Elcon charger: Wrong input voltage at AC plug!");
		}
		if (statusFlags & 8)
		{
			Logger::error("Elcon charger: No or reverse polarity battery. No charging!");
		}

		if ((statusFlags == 0)  && (outputVoltage < (settings.chargingVoltage - 350)))
		{
			wantCharging = true;
		}
		if (wantCharging) sendCommand();
	}
}

void ElconCharger::sendCommand()
{
	CAN_FRAME commandFrame;
	commandFrame.extended = true;
	commandFrame.id = 0x1806E5F4;
	commandFrame.length = 8;
	commandFrame.rtr = 0;

	static uint16_t targetAmperage = settings.chargingAmperage;
	uint16_t lowAmperage = settings.chargingAmperage / 10; //lowest amperage to ever request while charging

	int16_t voltageDifference = settings.chargingVoltage - outputVoltage; //remember, value unit is 0.1V

	targetAmperage = settings.chargingAmperage;

	if (voltageDifference <= 50)
	{
		int taper = (voltageDifference * 256) / 50;
		targetAmperage = lowAmperage + ((lowAmperage * taper * 9) / 256);
	}

	//If we have exceeded the voltage we were shooting for then abort the charge
	if (voltageDifference <= 0) wantCharging = false;

	//if the BMS has signaled that something is wrong (too hot, too cold, cell voltage too high, etc) then abort the charge
	if (status.CHARGE_OK == 0) wantCharging = false;

	if (statusFlags == 0 && (settings.currentPackAH < settings.maxPackAH) && wantCharging)
	{
		Logger::debug("Sending command to continue charging");
		commandFrame.data.bytes[0] = highByte(settings.chargingVoltage); //charging voltage high byte (in tenths)
		commandFrame.data.bytes[1] = lowByte(settings.chargingVoltage); //charging voltage low byte (in tenths)
		commandFrame.data.bytes[2] = highByte(targetAmperage); //charging current high byte (in tenths)
		commandFrame.data.bytes[3] = lowByte(targetAmperage); //charging current low byte (in tenths)
		commandFrame.data.bytes[4] = 0; //00 = charge, 1 = dont charge
		commandFrame.data.bytes[5] = 0; //reserved. send as 0
		commandFrame.data.bytes[6] = 0; //reserved. send as 0
		commandFrame.data.bytes[7] = 0; //reserved. send as 0
	}
	else
	{
		Logger::debug("Sending command to cease charging");
		commandFrame.data.bytes[0] = highByte(settings.chargingVoltage); //charging voltage high byte (in tenths)
		commandFrame.data.bytes[1] = lowByte(settings.chargingVoltage); //charging voltage low byte (in tenths)
		commandFrame.data.bytes[2] = 0; //charging current high byte (in tenths)
		commandFrame.data.bytes[3] = 0; //charging current low byte (in tenths)
		commandFrame.data.bytes[4] = 1; //00 = charge, 1 = dont charge
		commandFrame.data.bytes[5] = 0; //reserved. send as 0
		commandFrame.data.bytes[6] = 0; //reserved. send as 0
		commandFrame.data.bytes[7] = 0; //reserved. send as 0
	}
	Can0.sendFrame(commandFrame);
	Logger::debug("Sent frame to ELCON");
}

int32_t ElconCharger::getVoltage()
{
	return outputVoltage;
}

int32_t ElconCharger::getCurrent()
{
	return outputCurrent;
}
