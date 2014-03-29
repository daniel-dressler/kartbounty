#include <stdint.h>

#include "../../Standard.h"
#include "../entities/entities.h"

#include "input.h"

#define PLAYER_KART_INDEX 0

Input::Input() {
	m_Mailbox.request(Events::EventType::PlayerKart);
	m_Mailbox.request(Events::EventType::StartMenu);

	// Load database of joystick to controller mappings
	SDL_GameControllerAddMappingsFromFile("./assets/gamecontrollerdb.txt");

	// Disable Joystick updates over events
	SDL_JoystickEventState(SDL_IGNORE);
}

Input::~Input() {
	for (auto kart : m_players) { 
		SDL_HapticClose(kart.second->haptic);
		SDL_GameControllerClose(kart.second->controller);
		SDL_JoystickClose(kart.second->joystick);
	}
}

void Input::setup() {
	lastKbInput = NEWEVENT(Input);


	// Claim any jobsticks connected
	DEBUGOUT("%i joysticks found.\n", SDL_NumJoysticks() );
	DEBUGOUT("Number of haptic devices: %d\n", SDL_NumHaptics());
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		m_free_joysticks.push_back(i);
	}
}

/*
void Input::OpenJoysticks(){
		auto joystick = SDL_JoystickOpen(i);
		int number_of_buttons;
		number_of_buttons = SDL_JoystickNumButtons(joystick);
		DEBUGOUT("Joystick %s opened with %i buttons\n", SDL_JoystickName(joystick), number_of_buttons);
		m_mJoysticks.push_back(joystick);
	}
}
*/

void Input::HandleEvents() {

	std::vector<entity_id> kart_ids;
	Events::StartMenuInputEvent *menu = NULL;

	// Get Event Instructions
	for ( Events::Event *mail_event : (m_Mailbox.checkMail()) )
	{
		switch ( mail_event->type )
		{
		case Events::EventType::PlayerKart:
			{
				// What kart is this input for?
				auto kart_id = ((Events::PlayerKartEvent *)mail_event)->kart_id;
				kart_ids.push_back(kart_id);
				break;
			}
		case Events::EventType::StartMenu:
			{
				menu = NEWEVENT( StartMenuInput );
			}
			break;
		default:
			DEBUGOUT("Warning: Input ignored event of type %d\n", mail_event->type);
			break;
		}
	}

	// Get Keyboard input
	SDL_Event sdl_event;
	std::vector<Events::Event *> outEvents;
	Events::InputEvent *kbInput = NEWEVENT(Input);
	// Copy past kb input
	auto event_id = kbInput->id;
	memcpy(kbInput, &lastKbInput, sizeof(*kbInput));
	kbInput->id = event_id;
	while( SDL_PollEvent(&sdl_event) ) {
		switch (sdl_event.type) {
		case SDL_WINDOWEVENT:
			switch (sdl_event.window.event)
			{
			case SDL_WINDOWEVENT_RESIZED:
				SDL_Window* win;
				Int32 nWinWidth, nWinHeight;			
				win = SDL_GetWindowFromID(sdl_event.window.windowID);
				SDL_GetWindowSize( win, &nWinWidth, &nWinHeight );
				glViewport( 0, 0, nWinWidth, nWinHeight );
				break;
			default:
				break;
			}
			break;
		case SDL_QUIT:
			{
				outEvents.push_back( NEWEVENT( Quit ) );
			}
			break;
		case SDL_KEYDOWN:
			OnKeyDown(&sdl_event, &outEvents, kbInput);
			break;
		case SDL_KEYUP:
			OnKeyUp(&sdl_event, kbInput);
			break;
		default:
			//DEBUGOUT("SDL Event type:%i\n", sdl_event->type);
			break;
		}
	}
	// Save keyboard input
	memcpy(lastKbInput, kbInput, sizeof(*kbInput));

	// Menu always get kb input
	if (menu) {
		menu->aPressed = kbInput->aPressed;
		menu->bPressed = kbInput->bPressed;
		menu->xPressed = kbInput->xPressed;
		menu->yPressed = kbInput->yPressed;
	}

	// Gen local player data
	// Note: First players get controllers
	for (auto kart_id : kart_ids) {
		player_kart *kart_local = NULL;

		// Have we seen kart before?
		if (m_players.count(kart_id) == 0) {
			kart_local = new player_kart();

			kart_local->controller = NULL;
			kart_local->haptic = NULL;
			kart_local->joystick = NULL;

			m_players[kart_id] = kart_local;
		} else {
			kart_local = m_players[kart_id];
		}

		// Is kart matched to joystick yet?
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
	}


	// Gen input for active players
	// Note: Last player gets keyboard input
	bool kbInputTaken = false;
	for (int i = kart_ids.size() - 1; i >= 0; i--) {
		auto kart_id = kart_ids[i];
		auto kart_local = m_players[kart_id];

		// Input to be sent
		Events::InputEvent *input = NULL;
		bool took_kb = false;
		if (!kbInputTaken) {
			input = kbInput;

			took_kb = kbInputTaken = true;
		} else {
			NEWEVENT(Input);
		}

		// Reset Development Tools
		input->print_position = false;
		input->reset_requested = false;


		if (kart_local->controller) {
			PollController(kart_local->controller, &outEvents, input);
		} else if (kart_local->joystick) {
			PollJoystick(kart_local->joystick, &outEvents, input);
		}


		if (menu) {
			menu->aPressed |= input->aPressed;
			menu->bPressed |= input->bPressed;
			menu->xPressed |= input->xPressed;
			menu->yPressed |= input->yPressed;
		}


		// Send input
		outEvents.push_back(input);
		//puts("1here");
	}

	m_Mailbox.emptyMail();
	
	if (menu)
		outEvents.push_back(menu);
	m_Mailbox.sendMail(outEvents);
}

void Input::OnKeyDown(SDL_Event *event, std::vector<Events::Event *> *eventQueue, Events::InputEvent *outInput){
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

void Input::OnKeyUp(SDL_Event *event, Events::InputEvent *outInput) {
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


void Input::PollController(SDL_GameController *controller,
		std::vector<Events::Event *> *eventQueue, Events::InputEvent *out){

	#define POLL(button) \
		(1 == SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_ ## button))

	out->aPressed |= POLL(A);
	out->xPressed |= POLL(X);
	out->bPressed |= POLL(B);
	out->yPressed |= POLL(Y);

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
		((float)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_ ## axis)) / 32767

	// TODO: add deadzone && clamp
	out->leftTrigger      += POLL(TRIGGERLEFT);
	out->rightTrigger     += POLL(TRIGGERRIGHT);
	out->leftThumbStickRL += POLL(LEFTX);

	#undef POLL
}

void Input::PollJoystick(SDL_Joystick *joystick,
		std::vector<Events::Event *> *eventQueue, Events::InputEvent *out){

	#define POLL(button) \
		(1 == SDL_JoystickGetButton(joystick, button))

	out->aPressed |= POLL(A_BUTTON);
	out->xPressed |= POLL(X_BUTTON);
	out->bPressed |= POLL(B_BUTTON);
	out->yPressed |= POLL(Y_BUTTON);

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
		((float)SDL_JoystickGetAxis(joystick, axis)) / 32767.0

	// TODO: add deadzone
	out->leftTrigger      += POLL(LEFT_TRIGGER_AXIS);
	out->rightTrigger     += POLL(RIGHT_TRIGGER_AXIS);
	out->leftThumbStickRL += POLL(LEFT_STICK_LEFT_RIGHT_AXIS);

	#undef POLL
}

