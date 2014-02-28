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

	// Setup previous input event struct
	m_pPreviousInput = NEWEVENT(Input);
	m_pPreviousInput->rightTrigger = 0;
	m_pPreviousInput->leftTrigger = 0;
	m_pPreviousInput->leftThumbStickRL = 0;
	m_pPreviousInput->rightThumbStickRL = 0;
	m_pPreviousInput->aPressed = false;
	m_pPreviousInput->bPressed = false;
	m_pPreviousInput->xPressed = false;
	m_pPreviousInput->yPressed = false;
}

Input::~Input() {
	SDL_JoystickClose(m_joystick1);
	delete m_pPreviousInput;
	delete m_pMailbox;
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
	memcpy(m_pCurrentInput, m_pPreviousInput, sizeof(Events::InputEvent));

	// Inititialize input event with previous events values, except buttons
	//m_pCurrentInput->rightTrigger = 0;
	//m_pCurrentInput->leftTrigger = 0;
	//m_pCurrentInput->leftThumbStickRL = 0;
	//m_pCurrentInput->rightThumbStickRL = 0;
	//m_pCurrentInput->aPressed = false;
	//m_pCurrentInput->bPressed = false;
	//m_pCurrentInput->xPressed = false;
	//m_pCurrentInput->yPressed = false;

	//20ms into the future, ensures the input loop doesn't last longer than 10ms
	Uint32 timeout = SDL_GetTicks() + 20;   

	while( SDL_PollEvent(&event) )
	{
		OnEvent(&event);
	}

	// Update previous input event
	memcpy(m_pPreviousInput, m_pCurrentInput, sizeof(Events::InputEvent));

	// Send input events to mail system
	std::vector<Events::Event *> inputEvents;
	inputEvents.push_back(m_pCurrentInput);
	m_pMailbox->sendMail(inputEvents);
	
	// Now mailbox owns the object
	m_pCurrentInput = NULL;
}

void Input::OnEvent(SDL_Event* Event) 
{
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
		{
			std::vector<Events::Event *> quitEvent;
			quitEvent.push_back( NEWEVENT( Quit ) );
			m_pMailbox->sendMail( quitEvent );
		}
		break;
	case SDL_JOYBUTTONDOWN:
		OnJoystickButtonDown(Event->jbutton);
		break;
	case SDL_JOYBUTTONUP:
		OnJoystickButtonUp(Event->jbutton);
		break;
	case SDL_KEYDOWN:
		OnKeyDown(Event->key.keysym.sym, Event->key.keysym.mod, Event->key.type);
		break;
	case SDL_KEYUP:
		OnKeyUp(Event->key.keysym.sym, Event->key.keysym.mod, Event->key.type);
		break;
	case SDL_JOYAXISMOTION:
		OnJoystickAxisMotion(Event->jaxis);
		break;
	default:
		//DEBUGOUT("SDL Event type:%i\n", Event->type);
		break;
	}
}

void Input::OnKeyDown(SDL_Keycode keycode, Uint16 mod, Uint32 type){

	// Add key to state key map
	if(keycode < 256)
		GetMutState()->key_map[keycode] = true;

	// Handle key press for sending InputEvent to physics
	switch (keycode)
	{
	case SDLK_ESCAPE :	//This should pause the game but for now it just exits the program
		{
			std::vector<Events::Event *> quitEvent;
			quitEvent.push_back( NEWEVENT( Quit ) );
			m_pMailbox->sendMail( quitEvent );
			break;
		}
	case SDLK_a:
		m_pCurrentInput->leftThumbStickRL = -1;
		break;
	case SDLK_d:
		m_pCurrentInput->leftThumbStickRL = 1;
		break;
	case SDLK_RIGHT:
		m_pCurrentInput->leftThumbStickRL = 1;
		break;
	case SDLK_LEFT:
		m_pCurrentInput->leftThumbStickRL = -1;
		break;
	case SDLK_w:
		m_pCurrentInput->rightTrigger = 1;
		break;
	case SDLK_s:
		m_pCurrentInput->leftTrigger = 1;
		break;
	default:
		break;
	}
}

void Input::OnKeyUp(SDL_Keycode keycode, Uint16 mod, Uint32 type){
	if(keycode < 256)
		GetMutState()->key_map[keycode] = false;
	switch (keycode)
	{
	case SDLK_a:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	case SDLK_d:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	case SDLK_w:
		m_pCurrentInput->rightTrigger = 0;
		break;
	case SDLK_s:
		m_pCurrentInput->leftTrigger = 0;
		break;
	case SDLK_LEFT:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	case SDLK_RIGHT:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	default:
		break;
	}
}

void Input::OnJoystickAxisMotion(SDL_JoyAxisEvent event){

	// Used to scale joystick inputs to range of -1 to 1
	Sint16 scaleFactor = 32767;

	float moveAmt = (float)event.value / (float)scaleFactor;
	switch (event.axis)
	{
	case LEFT_STICK_LEFT_RIGHT_AXIS:
		if(abs(moveAmt) < 0.08)
			moveAmt = 0.0;
		m_pCurrentInput->leftThumbStickRL = moveAmt;
		break;
	case LEFT_STICK_UP_DOWN_AXIS:
		break;
	case RIGHT_STICK_LEFT_RIGHT_AXIS:
		break;
	case RIGHT_STICK_UP_DOWN_AXIS:
		break;
	case LEFT_TRIGGER_AXIS:
		{
		double leftTriggerValue = moveAmt;
		m_pCurrentInput->leftTrigger = (leftTriggerValue + 1) / 2;	//Scales the value to [0,1] instead of [-1,1]
		break;
		}
	case RIGHT_TRIGGER_AXIS:
		{
		double rightTriggerValue = moveAmt;
		m_pCurrentInput->rightTrigger = (rightTriggerValue + 1) / 2;  //Scales the value to [0,1] instead of [-1,1]			
		break;
		}
	default:
		break;
	}
}

void Input::OnJoystickButtonDown(SDL_JoyButtonEvent event){
	//DEBUGOUT("Joystick button: %i\n", event.button);
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
	case XBOX_BUTTON:
		{
		std::vector<Events::Event *> quitEvent;
		quitEvent.push_back( NEWEVENT( Quit ) );
		m_pMailbox->sendMail( quitEvent );
		break;
		}
	default:
		break;
	}
}

void Input::OnJoystickButtonUp(SDL_JoyButtonEvent event){
	//DEBUGOUT("Joystick button: %i\n", event.button);
	switch (event.button)
	{
	case A_BUTTON:
		m_pCurrentInput->aPressed = false;
		break;
	case X_BUTTON:
		m_pCurrentInput->xPressed = false;
		break;
	case B_BUTTON:
		m_pCurrentInput->bPressed = false;
		break;
	case Y_BUTTON:
		m_pCurrentInput->yPressed = false;
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
