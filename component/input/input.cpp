#include <stdint.h>

#include "../../Standard.h"
#include "../entities/entities.h"
#include "../state/state.h"

#include "input.h"

#define PLAYER_KART_INDEX 0

Input::Input() {
	m_pMailbox = new Events::Mailbox();	
	m_pMailbox->request(Events::EventType::PlayerKart);
}

Input::~Input() {
	for (auto joystick : m_mJoysticks) {
		SDL_JoystickClose(joystick);
	}
	for (auto haptic : m_mHaptics) { 
		SDL_HapticClose(haptic);
	}

	delete m_pPreviousInput;
	delete m_pMailbox;
}

void Input::setup() {
	// Initialize joysticks
	OpenJoysticks();

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
	m_pPreviousInput->print_position = false;
	m_pPreviousInput->reset_requested = false;
}

void Input::OpenJoysticks(){
	DEBUGOUT("%i joysticks found.\n", SDL_NumJoysticks() );
	DEBUGOUT("Number of haptic devices: %d\n", SDL_NumHaptics());

	SDL_JoystickEventState(SDL_ENABLE);
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		auto joystick = SDL_JoystickOpen(i);
		int number_of_buttons;
		number_of_buttons = SDL_JoystickNumButtons(joystick);
		DEBUGOUT("Joystick %s opened with %i buttons\n", SDL_JoystickName(joystick), number_of_buttons);
		m_mJoysticks.push_back(joystick);
	}
}

void Input::HandleEvents(){

	SDL_Event sdl_event;
	std::vector<Events::Event *> inputEvents;

	for ( Events::Event *mail_event : (m_pMailbox->checkMail()) )
	{
		switch ( mail_event->type )
		{
		case Events::EventType::PlayerKart:
		{
			m_pCurrentInput = NEWEVENT(Input);
			auto event_id = m_pCurrentInput->id;
			memcpy(m_pCurrentInput, m_pPreviousInput, sizeof(Events::InputEvent));
			m_pCurrentInput->id = event_id;

			// What kart is this input for?
			auto kart_id = ((Events::PlayerKartEvent *)mail_event)->kart_id;
			m_pCurrentInput->kart_id = kart_id;

			// Reset Development Tools
			m_pCurrentInput->print_position = false;
			m_pCurrentInput->reset_requested = false;

			while( SDL_PollEvent(&sdl_event) )
			{
				OnEvent(&sdl_event);
			}

			// Update previous input event
			memcpy(m_pPreviousInput, m_pCurrentInput, sizeof(Events::InputEvent));

			// Send input events to mail system
			inputEvents.push_back(m_pCurrentInput);

			// Now mailbox owns the object
			m_pCurrentInput = NULL;
			break;
		}
		default:
			break;
		}
	}
	m_pMailbox->emptyMail();

	m_pMailbox->sendMail(inputEvents);
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
	case SDLK_SPACE:
		m_pCurrentInput->bPressed = true;
		break;
	case SDLK_LSHIFT:
		m_pCurrentInput->aPressed = true;
		break;

		// Development Tools
	case SDLK_z:
		m_pCurrentInput->print_position = true;
		break;
	case SDLK_r:
		m_pCurrentInput->reset_requested = true;
		break;
	case SDLK_BACKSPACE:
		{
			std::vector<Events::Event *> resetEvent;
			resetEvent.push_back( NEWEVENT( RoundStart) );
			m_pMailbox->sendMail(resetEvent);
		}
		break;
	default:
		break;
	}
}

void Input::OnKeyUp(SDL_Keycode keycode, Uint16 mod, Uint32 type){
	switch (keycode)
	{
	case SDLK_a:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	case SDLK_d:
		m_pCurrentInput->leftThumbStickRL = 0;
		break;
	case SDLK_w:
		m_pCurrentInput->rightTrigger = 0.01;
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
	case SDLK_SPACE:
		m_pCurrentInput->bPressed = false;
		break;
	case SDLK_LSHIFT:
		m_pCurrentInput->aPressed = false;
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
		float leftTriggerValue = moveAmt;
		m_pCurrentInput->leftTrigger = (leftTriggerValue + 1) / 2;	//Scales the value to [0,1] instead of [-1,1]
		break;
		}
	case RIGHT_TRIGGER_AXIS:
		{
		float rightTriggerValue = moveAmt;
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
		{
			std::vector<Events::Event *> pauseEvent;
			pauseEvent.push_back( NEWEVENT( TogglePauseGame ) );
			m_pMailbox->sendMail( pauseEvent );
		}
		break;
	case BACK_BUTTON:
		{
			//std::vector<Events::Event *> resetEvent;
			//resetEvent.push_back( NEWEVENT( RoundStart) );
			//m_pMailbox->sendMail(resetEvent);
		}
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
