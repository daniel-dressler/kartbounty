#pragma once

#include <vector>
#include <SDL.h>
#include <algorithm>

#include "../events/events.h"

class Input {

private:
	Events::Mailbox m_Mailbox;

	struct player_kart {
		SDL_Joystick *joystick;
		SDL_Haptic *haptic;
		SDL_GameController *controller;
		SDL_JoystickID joystick_id;
	};
	std::map<entity_id, player_kart *>m_players;
	std::map<int, entity_id>m_taken_joysticks;
	std::vector<int>m_free_joysticks;

	Events::InputEvent *lastKbInput;

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
	int static const XBOX_BUTTON = 14;

	// X-BOX controller joystick axis mappings
	int static const LEFT_STICK_LEFT_RIGHT_AXIS = 0;
	int static const LEFT_STICK_UP_DOWN_AXIS = 1;
	int static const RIGHT_STICK_LEFT_RIGHT_AXIS = 2;
	int static const RIGHT_STICK_UP_DOWN_AXIS = 3;
	int static const LEFT_TRIGGER_AXIS = 4;
	int static const RIGHT_TRIGGER_AXIS = 5;

#define MIN_JOY_MOVEMENT_THRESHOLD 500
#define JOYSTICK_DEADZONE 3000

	Input();

	~Input();

	void setup();

	void HandleEvents();

	// Keyboard events
	void OnKeyDown(SDL_Event* Event, std::vector<Events::Event *> *, Events::InputEvent *);
	void OnKeyUp(SDL_Event* Event, Events::InputEvent *);

	// Joysticks & Controllers
	void PollController(SDL_GameController *, std::vector<Events::Event *> *, Events::InputEvent *);
	void PollJoystick(SDL_Joystick *, std::vector<Events::Event *> *, Events::InputEvent *);

};
