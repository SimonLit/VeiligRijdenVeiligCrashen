#ifndef CRASH_H
#define CRASH_H

#include "stdint.h"
#include "RP6I2CmasterTWI.h"
#include "RP6uart.h"
#include "RP6Control_I2CMasterLib.h"

extern uint8_t hitSide;
extern uint8_t pressed;
extern uint8_t crashInfoWasSend;

typedef struct
{
	double speed;
	uint8_t sideHit;
	uint16_t impactGram;
	uint16_t distanceDrivenInCM;
}crashInfo;

int assignCrashInfo(crashInfo* cInfo);

void sendCrashInfo(crashInfo* cInfo);

void buttenChanged(void);

void task_checkButtonChanged(void);

#endif
