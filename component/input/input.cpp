#include <stdint.h>
#include "input.h"
#include "../../Standard.h"
#include "../events/events.h"
#include "../state/state.h"

Input::Input() {
	m_pMailbox = new Events::Mailbox();	
	
	// Initialize joysticks
	DEBUGOUT("%i joysticks found.\n", SDL_NumJoysticks() );
	if(SDL_NumJoysticks())
	{
		OpenJoysticks();
	}
	newInputs = false;
}

Input::~Input() {
	SDL_JoystickClose(m_joystick1);
}

void Input::OpenJoysticks(){
	SDL_JoystickEventState(SDL_ENABLE);
	m_joystick1 = SDL_JoystickOpen(0);
	int number_of_buttons;
	number_of_buttons = SDL_JoystickNumButtons(m_joystick1);
	DEBUGOUT("Joystick %i opened with %i buttons\n", m_joystick1, number_of_buttons);
}

void Input::HandleEvents(){

	SDL_Event event;

	m_pCurrentInput = NEWEVENT(Input);
	//memset(m_pCurrentInput, 0, sizeof(struct Events::InputEvent));  //This resets everything including the type and id, not good
	
	// Make sure all values are set to 0.
	m_pCurrentInput->accelerate = 0;
	m_pCurrentInput->brake = 0;
	m_pCurrentInput->turn = 0;
	m_pCurrentInput->aPressed = false;
	m_pCurrentInput->bPressed = false;
	m_pCurrentInput->xPressed = false;
	m_pCurrentInput->yPressed = false;

	//20ms into the future, ensures the input loop doesn't last longer than 10ms
	Uint32 timeout = SDL_GetTicks() + 20;   

	while( SDL_PollEvent(&event) && !SDL_TICKS_PASSED(SDL_GetTicks(), timeout) )
	{
		OnEvent(&event);
	}

	if(newInputs)
	{
		// Send input events to mail system
		std::vector<Events::Event *> inputEvents;
		inputEvents.push_back(m_pCurrentInput);
		m_pMailbox->sendMail(inputEvents);
	}
	
	// Now mailbox owns the object
	m_pCurrentInput = NULL;
	newInputs = false;
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
		//DEBUGOUT("SDL Event type:%i\n", Event->type);
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
		newInputs = true;
		switch (event.axis)
		{
		case LEFT_STICK_LEFT_RIGHT_AXIS:
			m_pCurrentInput->turn = max(event.value / scaleFactor, m_pCurrentInput->turn);
			break;
		case LEFT_STICK_UP_DOWN_AXIS:
			break;
		case RIGHT_STICK_LEFT_RIGHT_AXIS:
			break;
		case RIGHT_STICK_UP_DOWN_AXIS:
			break;
		case LEFT_TRIGGER_AXIS:
			m_pCurrentInput->brake = max(event.value / scaleFactor, m_pCurrentInput->brake);
			break;
		case RIGHT_TRIGGER_AXIS:
			m_pCurrentInput->accelerate = max(event.value / scaleFactor, m_pCurrentInput->accelerate);
			break;
		default:
			break;
		}
	}

}

void Input::OnJoystickButton(SDL_JoyButtonEvent event){
	//DEBUGOUT("Joystick button: %i\n", event.button);
	newInputs = true;
	switch (event.button)
	{
	case A_BUTTON:
		m_pCurrentInput->aPressed = true;
		break;
	case X_BUTTON:
		m_pCurrentInput->xPressed = true;
		break;
	case B_BUTTON:
		m_pCurrentInput->bPressed = true;
		break;
	case Y_BUTTON:
		m_pCurrentInput->yPressed = true;
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
