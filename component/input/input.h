#pragma once

#include <SDL.h>

class Input {
public:

	SDL_Joystick *joystick1;

	Input();

	~Input();

	void OpenJoysticks();

	void OnEvent(SDL_Event* Event);

	void OnKeyDown(SDL_Scancode scancode, Uint16 mod, Uint32 type);

	void OnKeyUp(SDL_Scancode scancode, Uint16 mod, Uint32 type);

	void OnControllerAxisMotion(SDL_ControllerAxisEvent event);

};