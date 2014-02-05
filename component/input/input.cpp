#include <stdint.h>
#include "input.h"
#include "../../Standard.h"

Input::Input() {	
}

Input::~Input() {
	SDL_JoystickClose(joystick1);
}

void Input::OpenJoysticks(){
	SDL_JoystickEventState(SDL_ENABLE);
	joystick1 = SDL_JoystickOpen(0);
}

void Input::OnEvent(SDL_Event* Event) {
	switch (Event->type)
	{
	case SDL_WINDOWEVENT:
		switch (Event->window.event)
		{
			case SDL_WINDOWEVENT_RESIZED:
				SDL_Window* win;
				Int32 nWinWidth, nWinHeight;			
				win = SDL_GetWindowFromID(Event->window.windowID);
				SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );
				glViewport( 0, 0, nWinWidth, nWinHeight );
				break;
			default:
				break;
		}
		break;
	case SDL_QUIT:
		exit(0);
		break;
	case SDL_KEYDOWN:
		OnKeyDown(Event->key.keysym.scancode, Event->key.keysym.mod, Event->key.type);
		break;
	case SDL_KEYUP:
		OnKeyUp(Event->key.keysym.scancode, Event->key.keysym.mod, Event->key.type);
		break;
	//case SDL_JOYAXISMOTION:
	//	OnControllerAxisMotion(Event->jaxis.axis);
	//case SDL_CONTROLLERAXISMOTION:
	//	OnControllerAxisMotion(Event->caxis);
	//	break;
	default:
		//DEBUGOUT("SDL Event type:%i", Event->type);
		break;
	}
}

void Input::OnKeyDown(SDL_Scancode scancode, Uint16 mod, Uint32 type){
	switch (scancode)
	{
	case SDL_SCANCODE_ESCAPE:	//This should pause the game but for now it just exits the program
		{
			exit(0);
			break;
		}
	default:
		break;
	}
}

void Input::OnKeyUp(SDL_Scancode scancode, Uint16 mod, Uint32 type){
	switch (scancode)
	{
	default:
		break;
	}
}

void Input::OnControllerAxisMotion(SDL_ControllerAxisEvent event){
	int x = 0;
	x++;
}