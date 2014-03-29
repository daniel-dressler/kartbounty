#include <stdint.h>

#include "../../Standard.h"
#include "../entities/entities.h"

#include "input.h"

#define PLAYER_KART_INDEX 0

Input::Input() {
	m_pMailbox = new Events::Mailbox();	
	m_pMailbox->request(Events::EventType::PlayerKart);
	m_pMailbox->request(Events::EventType::StartMenu);

	// Load database of joystick to controller mappings
	SDL_GameControllerAddMappingsFromFile("./assets/gamecontrollerdb.txt");

	// Disable Joystick updates over events
	//SDL_JoystickEventState(SDL_IGNORE);
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
}

void Input::OpenJoysticks(){
	DEBUGOUT("%i joysticks found.\n", SDL_NumJoysticks() );
	DEBUGOUT("Number of haptic devices: %d\n", SDL_NumHaptics());

	SDL_JoystickEventState(SDL_ENABLE);
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		m_free_joysticks.push_back(i);

		auto joystick = SDL_JoystickOpen(i);
		int number_of_buttons;
		number_of_buttons = SDL_JoystickNumButtons(joystick);
		DEBUGOUT("Joystick %s opened with %i buttons\n", SDL_JoystickName(joystick), number_of_buttons);
		m_mJoysticks.push_back(joystick);
	}
}

void Input::HandleEvents(){

	SDL_Event sdl_event;
	std::vector<Events::Event *> outEvents;

	for ( Events::Event *mail_event : (m_pMailbox->checkMail()) )
	{
		switch ( mail_event->type )
		{
		case Events::EventType::PlayerKart:
			{
				// What kart is this input for?
				auto kart_id = ((Events::PlayerKartEvent *)mail_event)->kart_id;

				// Is kart matched to joystick yet?
				player_kart *kart_local = NULL;
				if (m_players.count(kart_id) == 0) {
					player_kart *kart_local = new player_kart();
					m_players[kart_id] = kart_local;
					kart_local->last_input = NEWEVENT(Input);
					kart_local->controller = NULL;
					kart_local->haptic = NULL;
					kart_local->joystick = NULL;
				} else {
					kart_local = m_players[kart_id];
				}

				// Does player have a controller?
				if (kart_local->joystick == NULL) {
					// Get next free joysick
					if (m_free_joysticks.size() > 0) {
						int free_joystick = m_free_joysticks[0];
						m_free_joysticks.erase(m_free_joysticks.begin());
						kart_local->joystick = SDL_JoystickOpen(free_joystick);
						kart_local->haptic = SDL_HapticOpenFromJoystick(kart_local->joystick);
						if (SDL_IsGameController(free_joystick)) {
							kart_local->controller = SDL_GameControllerOpen(free_joystick);
						}

						m_taken_joysticks[free_joystick] = kart_id;
					} else {
						DEBUGOUT("Warning: Could not get joystick for player of kart %d\n", kart_id);
					}
				}

				// Input to be sent
				auto input = NEWEVENT(Input);
				auto event_id = input->id;
				memcpy(input, kart_local->last_input, sizeof(*input));
				input->id = event_id;

				// Reset Development Tools
				input->print_position = false;
				input->reset_requested = false;

				while( SDL_PollEvent(&sdl_event) )
				{
					OnEvent(&sdl_event, &outEvents, input);
				}

				if (kart_local->controller) {
					PollController(kart_local->controller, &outEvents, input);
				} else if (kart_local->joystick) {
					PollJoystick(kart_local->joystick, &outEvents, input);
				}

				// Save current input
				memcpy(kart_local->last_input, input, sizeof(*input));

				// Send input
				outEvents.push_back(input);

				break;
			}
		case Events::EventType::StartMenu:
			{
				// TODO: Daniel, fix this
				auto input = NEWEVENT(Input);

				while( SDL_PollEvent(&sdl_event) )
				{
					OnEvent(&sdl_event, &outEvents, input);
				}

				auto menuInput = NEWEVENT( StartMenuInput );
				menuInput->aPressed = m_pCurrentInput->aPressed;
				menuInput->bPressed = m_pCurrentInput->bPressed;
				menuInput->xPressed = m_pCurrentInput->xPressed;
				menuInput->yPressed = m_pCurrentInput->yPressed;

				// Send input events to mail system
				outEvents.push_back(input);
			}
			break;
		default:
			break;
		}
	}
	m_pMailbox->emptyMail();

	m_pMailbox->sendMail(outEvents);
}

void Input::OnEvent(SDL_Event* event, std::vector<Events::Event *> *eventQueue, Events::InputEvent *outInput) 
{
	switch (event->type)
	{
	case SDL_WINDOWEVENT:
		switch (event->window.event)
		{
		case SDL_WINDOWEVENT_RESIZED:
			SDL_Window* win;
			Int32 nWinWidth, nWinHeight;			
			win = SDL_GetWindowFromID(event->window.windowID);
			SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );
			glViewport( 0, 0, nWinWidth, nWinHeight );
			break;
		default:
			break;
		}
		break;
	case SDL_QUIT:
		{
			eventQueue->push_back( NEWEVENT( Quit ) );
		}
		break;
	case SDL_JOYBUTTONDOWN:
		OnJoystickButtonDown(event->jbutton);
		break;
	case SDL_JOYBUTTONUP:
		OnJoystickButtonUp(event->jbutton);
		break;
	case SDL_KEYDOWN:
		OnKeyDown(event, eventQueue, outInput);
		break;
	case SDL_KEYUP:
		OnKeyUp(event, eventQueue, outInput);
		break;
	case SDL_JOYAXISMOTION:
		OnJoystickAxisMotion(event->jaxis);
		break;
	default:
		//DEBUGOUT("SDL Event type:%i\n", Event->type);
		break;
	}
}

void Input::OnKeyDown(SDL_Event* event, std::vector<Events::Event *> *eventQueue, Events::InputEvent *outInput){
	SDL_Keycode keycode = event->key.keysym.sym;

	// Handle key press for sending InputEvent to physics
	switch (keycode)
	{
	case SDLK_ESCAPE :	//This should pause the game but for now it just exits the program
		{
			eventQueue->push_back( NEWEVENT( Quit ) );
			break;
		}
	case SDLK_a:
		outInput->leftThumbStickRL = -1;
		break;
	case SDLK_d:
		outInput->leftThumbStickRL = 1;
		break;
	case SDLK_RIGHT:
		outInput->leftThumbStickRL = 1;
		break;
	case SDLK_LEFT:
		outInput->leftThumbStickRL = -1;
		break;
	case SDLK_w:
		outInput->rightTrigger = 1;
		break;
	case SDLK_s:
		outInput->leftTrigger = 1;
		break;
	case SDLK_SPACE:
		outInput->bPressed = true;
		break;
	case SDLK_LSHIFT:
		outInput->aPressed = true;
		break;
	case SDLK_RETURN:
		outInput->xPressed = true;
		break;

		// Development Tools
	case SDLK_z:
		outInput->print_position = true;
		break;
	case SDLK_r:
		outInput->reset_requested = true;
		break;
	default:
		break;
	}
}

void Input::OnKeyUp(SDL_Event* event, std::vector<Events::Event *> *eventQueue, Events::InputEvent *outInput){
	SDL_Keycode keycode = event->key.keysym.sym;

	switch (keycode)
	{
	case SDLK_a:
		outInput->leftThumbStickRL = 0;
		break;
	case SDLK_d:
		outInput->leftThumbStickRL = 0;
		break;
	case SDLK_w:
		outInput->rightTrigger = 0.01;
		break;
	case SDLK_s:
		outInput->leftTrigger = 0;
		break;
	case SDLK_LEFT:
		outInput->leftThumbStickRL = 0;
		break;
	case SDLK_RIGHT:
		outInput->leftThumbStickRL = 0;
		break;
	case SDLK_SPACE:
		outInput->bPressed = false;
		break;
	case SDLK_LSHIFT:
		outInput->aPressed = false;
		break;
	case SDLK_RETURN:
		outInput->xPressed = false;
		break;
	default:
		break;
	}
}


void Input::PollController(SDL_GameController* controller,
		std::vector<Events::Event *> *eventQueue, Events::InputEvent *out){

	#define POLL(button) \
		(1 == SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_ ## button))

	out->aPressed = POLL(A);
	out->xPressed = POLL(X);
	out->bPressed = POLL(B);
	out->yPressed = POLL(Y);

	if (POLL(START)) {
		eventQueue->push_back(NEWEVENT(TogglePauseGame));
	}

	if (POLL(BACK)) {
		//eventQueue.push_back(NEWEVENT(RoundStart));
	}

	if (POLL(GUIDE)) {
		eventQueue->push_back(NEWEVENT(Quit));
	}

	#undef POLL
	#define POLL(axis) \
		SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_ ## axis)

	// TODO: add deadzone
	out->leftTrigger = POLL(TRIGGERLEFT);
	out->rightTrigger = POLL(TRIGGERRIGHT);
	out->leftThumbStickRL = POLL(LEFTX);

	#undef POLL
}

void Input::PollJoystick(SDL_Joystick* joystick,
		std::vector<Events::Event *> *eventQueue, Events::InputEvent *out){

	#define POLL(button) \
		(1 == SDL_JoystickGetButton(joystick, button))

	out->aPressed = POLL(A_BUTTON);
	out->xPressed = POLL(X_BUTTON);
	out->bPressed = POLL(B_BUTTON);
	out->yPressed = POLL(Y_BUTTON);

	if (POLL(START_BUTTON)) {
		eventQueue->push_back(NEWEVENT(TogglePauseGame));
	}

	if (POLL(BACK_BUTTON)) {
		//eventQueue->push_back(NEWEVENT(RoundStart));
	}

	if (POLL(XBOX_BUTTON)) {
		eventQueue->push_back(NEWEVENT(Quit));
	}

	#undef POLL
	#define POLL(axis) \
		SDL_JoystickGetAxis(joystick, axis)

	// TODO: add deadzone
	out->leftTrigger = POLL(LEFT_TRIGGER_AXIS);
	out->rightTrigger = POLL(RIGHT_TRIGGER_AXIS);
	out->leftThumbStickRL = POLL(LEFT_STICK_LEFT_RIGHT_AXIS);

	#undef POLL
}

void Input::OnJoystickAxisMotion(SDL_JoyAxisEvent event){
	return;

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
	return;
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
