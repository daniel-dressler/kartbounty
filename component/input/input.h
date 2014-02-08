#pragma once

#include <SDL.h>
#include <algorithm>
#include "../events/events.h"

class Input {

private:

	SDL_Joystick *m_joystick1;
	Events::Mailbox *m_pMailbox;
	Events::InputEvent *m_pCurrentInput;
	bool newInputs;

public:

	// X-BOX controller button mappings
	int static const D_PAD_UP_BUTTON = 0;
	int static const D_PAD_DOWN_BUTTON = 1;
	int static const D_PAD_LEFT_BUTTON = 2;
	int static const D_PAD_RIGHT_BUTTON = 3;
	int static const START_BUTTON = 4;
	int static const BACK_BUTTON = 5;
	int static const LEFT_THUMB_STICK_BUTTON = 6;	
	int static const RIGHT_THUMB_STICK_BUTTON = 7;
	int static const LEFT_SHOULDER_BUTTON = 8;
	int static const RIGHT_SHOULDER_BUTTON = 9;
	int static const A_BUTTON = 10;
	int static const B_BUTTON = 11;
	int static const X_BUTTON = 12;
	int static const Y_BUTTON = 13;

	// X-BOX controller joystick axis mappings
	int static const LEFT_STICK_LEFT_RIGHT_AXIS = 0;
	int static const LEFT_STICK_UP_DOWN_AXIS = 1;
	int static const RIGHT_STICK_LEFT_RIGHT_AXIS = 2;
	int static const RIGHT_STICK_UP_DOWN_AXIS = 3;
	int static const LEFT_TRIGGER_AXIS = 4;
	int static const RIGHT_TRIGGER_AXIS = 5;

	int static const MIN_JOY_MOVEMENT_THRESHOLD = 3200;

	Input();

	~Input();

	void OpenJoysticks();

	void HandleEvents();
	void OnEvent(SDL_Event* Event);

	// Keyboard events
	void OnKeyDown(SDL_Scancode scancode, Uint16 mod, Uint32 type);
	void OnKeyUp(SDL_Scancode scancode, Uint16 mod, Uint32 type);

	//Joystick Events
	void OnJoystickAxisMotion(SDL_JoyAxisEvent event);
	void OnJoystickButton(SDL_JoyButtonEvent event);

};
