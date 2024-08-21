#ifndef FRAUS_INPUT_H
#define FRAUS_INPUT_H

#include <stdint.h>

typedef struct FrGamepad
{
	float leftStickX;
	float leftStickY;

	float rightStickX;
	float rightStickY;

	float leftTrigger;
	float rightTrigger;

	uint16_t leftBumper: 1;
	uint16_t rightBumper: 1;
	uint16_t leftStick: 1;
	uint16_t rightStick: 1;
	uint16_t crossUp: 1;
	uint16_t crossDown: 1;
	uint16_t crossLeft: 1;
	uint16_t crossRight: 1;
	uint16_t buttonUp: 1;
	uint16_t buttonDown: 1;
	uint16_t buttonLeft: 1;
	uint16_t buttonRight: 1;
	uint16_t optionsLeft: 1;
	uint16_t optionsRight: 1;

	uint16_t available: 1;
} FrGamepad;

extern FrGamepad frGamepad;

void frUpdateGamepad(void);

#endif
