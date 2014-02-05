#include <stdint.h>
#include "input.h"
#include "../../Standard.h"
#include "../events/events.h"
#include "../state/state.h"

Input::Input() {	
}

Input::~Input() {
	SDL_JoystickClose(m_joystick1);

	m_pMailbox = new Events::Mailbox();
}

void Input::OpenJoysticks(){
	SDL_JoystickEventState(SDL_ENABLE);
	m_joystick1 = SDL_JoystickOpen(0);
	int number_of_buttons;
	number_of_buttons = SDL_JoystickNumButtons(m_joystick1);
	DEBUGOUT("Joystick %i opened with %i buttons", m_joystick1, number_of_buttons);
}

void Input::HandleEvents(){

	SDL_Event event;
	Events::Event *outgoingInputEvents = new Events::Event(Events::EventType::KartMove);

	//10ms into the future, ensures the input loop doesn't last longer than 10ms
	Uint32 timeout = SDL_GetTicks() + 20;   

	while( SDL_PollEvent(&event) && !SDL_TICKS_PASSED(SDL_GetTicks(), timeout) )
	{
		OnEvent(&event);

		//Add code to handle compiling events into one outgoing mail item
	}
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
	case SDL_JOYAXISMOTION:
		OnJoystickAxisMotion(Event->jaxis);
		break;
	case SDL_JOYBUTTONDOWN:
		OnJoystickButton(Event->jbutton);
		break;
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

void Input::OnJoystickAxisMotion(SDL_JoyAxisEvent event){

	// Used to scale joystick inputs to range of -1 to 1
	Sint16 scaleFactor = 32767;

	if( (event.value >= MIN_JOY_MOVEMENT_THRESHOLD ) || (event.value <= -MIN_JOY_MOVEMENT_THRESHOLD) )
	{
		switch (event.axis)
		{
		case LEFT_STICK_LEFT_RIGHT_AXIS:
			break;
		case LEFT_STICK_UP_DOWN_AXIS:
			break;
		case RIGHT_STICK_LEFT_RIGHT_AXIS:
			break;
		case RIGHT_STICK_UP_DOWN_AXIS:
			break;
		case LEFT_TRIGGER_AXIS:
			break;
		case RIGHT_TRIGGER_AXIS:
			break;
		default:
			break;
		}
	}

}

void Input::OnJoystickButton(SDL_JoyButtonEvent event){
	//DEBUGOUT("Joystick button: %i\n", event.button);
	switch (event.button)
	{
	case A_BUTTON:
		break;
	case X_BUTTON:
		break;
	case B_BUTTON:
		break;
	case Y_BUTTON:
		break;
	case LEFT_SHOULDER_BUTTON:
		break;
	case RIGHT_SHOULDER_BUTTON:
		break;
	case START_BUTTON:
		break;
	case BACK_BUTTON:
		break;
	default:
		break;
	}
}