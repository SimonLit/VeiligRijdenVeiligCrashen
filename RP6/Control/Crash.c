#include "Crash.h" 
#include "Drive.h"
#include "RP6I2CmasterTWI.h"
#include "SerialReaderSender.h"
#include "Adc.h"
#include "RP6Control_I2CMasterLib.h"
#include "Tools.h"
#include "ProtocolDefines.h"

#include "RP6ControlLib.h"

//====================================================================================
// Crash
//====================================================================================

uint8_t pressed = false;
uint8_t crashInfoWasSend = false;
uint8_t hitSide = 0;

uint8_t lastButton2State = false;
uint8_t lastButton3State = false;
uint8_t lastButton5State = false;

// Used for the converting from Newton to grams.
float earthAcceleration = 9.81; 

char valueBuffer[MAX_VALUE_LENGTH];

int assignCrashInfo(crashInfo* cInfo)
{
	if(cInfo == NULL)
	{
		return -1;
	}

	uint16_t averageSpeed = calculateAverageSpeed();
	
	// One speed point is equal to 5 segments per second.
	// By multiplying the speed by 5
	// you get the amount of segments past
	// on the encoder per second. 
	// Each segment is +/- 0.24mm (= ENCODER_RESOLUTION). 
	// ENCODER_RESOLUTION is defined in the RP6Config.h file.
	// So by multiplying is by 0.025cm you get the amount of cm/s.
	uint8_t speedCMPerSecond = averageSpeed * 5 * ENCODER_RESOLUTION; 	

	cInfo->speed = speedCMPerSecond;
	cInfo->sideHit = hitSide;
	cInfo->impactGram = (mapPressureSensorValueToNewton(readADC(ADC_5))/earthAcceleration) * 1000;
	cInfo->distanceDrivenInCM = ((mleft_dist * (ENCODER_RESOLUTION/10)) + (mright_dist * (ENCODER_RESOLUTION/10))) / 2;

	return 0;
}

int sendCrashInfo(crashInfo* cInfo)
{
	if(cInfo == NULL){return -1;}
	uint8_t timeoutResult = 0;

	uint8_tToString(cInfo->speed, valueBuffer, sizeof(valueBuffer)); // Convert the speed to string.
	sendMessageWithValue(SPEED_PROTOCOL_SEND, valueBuffer);
	timeoutResult = timeoutHandler();

	uint8_tToString(cInfo->sideHit, valueBuffer, sizeof(valueBuffer)); // Convert the hit side to string.
	if(timeoutResult == 0)
	{	
		sendMessageWithValue(SIDE_HIT_PROTOCOL_SEND_RECEIVE, valueBuffer);
		timeoutResult = timeoutHandler();
	} 

	uint16_tToString(cInfo->impactGram, valueBuffer, sizeof(valueBuffer)); // Convert the impact in grams to string.
	if(timeoutResult == 0)
    {
        sendMessageWithValue(IMPACT_PROTOCOL_SEND_RECEIVE, valueBuffer);
        timeoutResult = timeoutHandler();
    } 

	uint16_tToString(cInfo->distanceDrivenInCM, valueBuffer, sizeof(valueBuffer)); // Convert the distance driven to string.
	if(timeoutResult == 0)
    {
        sendMessageWithValue(DIST_DRIVEN_PROTOCOL_SEND_RECEIVE, valueBuffer);
        timeoutResult = timeoutHandler();
    } 

	if(timeoutResult == 0)
    {
        sendMessage(ORIENTATION_PROTOCOL_RECEIVE);
        timeoutResult = timeoutHandler();
    } 

	crashInfoWasSend = true;
	return timeoutResult;
}

void buttenChanged(void)
{
	if(!pressed)
	{
	 	if(bumper_left || bumper_right)
	 	{
	 		hitSide = 1;
	 	}
	 	else if(PINC & IO_PC2)
	 	{
	 		hitSide = 4;
	 	}
	 	else if(PINC & IO_PC3)
	 	{
	 		hitSide = 3;
	 	}
	 	else if(PINC & IO_PC5)
	 	{
	 		hitSide = 2;
	 	}
	 	
		pressed = true;
	}
	else
	{
		pressed = false;
		crashInfoWasSend = false;
	}
}

int task_checkButtonChanged(void)
{
	if(getStopwatch6() > 50)
	{
		uint8_t button2State = PINC & IO_PC2;
		uint8_t button3State = PINC & IO_PC3;
		uint8_t button5State = PINC & IO_PC5;

		if(button2State !=  lastButton2State)
		{
			buttenChanged();
			lastButton2State = button2State;

			return 0;
		}	

		else if(button3State !=  lastButton3State)
		{
			buttenChanged();
			lastButton3State = button3State;

			return 0;
		}	

		else if(button5State !=  lastButton5State)
		{
			buttenChanged();
			lastButton5State = button5State;

			return 0;
		}	

		setStopwatch6(0);
	}

	return -1;
}
