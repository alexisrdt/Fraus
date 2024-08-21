#include "fraus/input.h"

#include <stdbool.h>

#include <windows.h>
#include <xinput.h>

FrGamepad frGamepad;

void frUpdateGamepad(void)
{
	XINPUT_STATE state;
	if(XInputGetState(0, &state) != ERROR_SUCCESS)
	{
		frGamepad.available = 0;
		return;
	}

	frGamepad.available = 1;

	const bool leftStickMagnitude = state.Gamepad.sThumbLX * state.Gamepad.sThumbLX + state.Gamepad.sThumbLY * state.Gamepad.sThumbLY >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	frGamepad.leftStickX = (float)state.Gamepad.sThumbLX / INT16_MAX * leftStickMagnitude;
	frGamepad.leftStickX = frGamepad.leftStickX < -1.f ? -1.f : frGamepad.leftStickX;
	frGamepad.leftStickY = (float)state.Gamepad.sThumbLY / INT16_MAX * leftStickMagnitude;
	frGamepad.leftStickY = frGamepad.leftStickY < -1.f ? -1.f : frGamepad.leftStickY;

	const bool rightStickMagnitude = state.Gamepad.sThumbRX * state.Gamepad.sThumbRX + state.Gamepad.sThumbRY * state.Gamepad.sThumbRY >= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	frGamepad.rightStickX = (float)state.Gamepad.sThumbRX / INT16_MAX * rightStickMagnitude;
	frGamepad.rightStickX = frGamepad.rightStickX < -1.f ? -1.f : frGamepad.rightStickX;
	frGamepad.rightStickY = (float)state.Gamepad.sThumbRY / INT16_MAX * rightStickMagnitude;
	frGamepad.rightStickY = frGamepad.rightStickY < -1.f ? -1.f : frGamepad.rightStickY;

	frGamepad.leftTrigger = (float)state.Gamepad.bLeftTrigger / UINT8_MAX * (state.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	frGamepad.rightTrigger = (float)state.Gamepad.bRightTrigger / UINT8_MAX * (state.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

	frGamepad.leftBumper = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) > 0;
	frGamepad.rightBumper = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) > 0;

	frGamepad.leftStick = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) > 0;
	frGamepad.rightStick = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) > 0;

	frGamepad.crossUp = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) > 0;
	frGamepad.crossDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) > 0;
	frGamepad.crossLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) > 0;
	frGamepad.crossRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) > 0;

	frGamepad.buttonUp = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) > 0;
	frGamepad.buttonDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) > 0;
	frGamepad.buttonLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) > 0;
	frGamepad.buttonRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) > 0;

	frGamepad.optionsLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) > 0;
	frGamepad.optionsRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) > 0;
}
