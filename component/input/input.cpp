#include <stdint.h>

#include "../../Standard.h"
#include "../entities/entities.h"

#include "input.h"

Input::Input() {
	m_Mailbox.request(Events::EventType::PlayerKart);
	m_Mailbox.request(Events::EventType::KartDestroyed);
	m_Mailbox.request(Events::EventType::StartMenu);

	// Load database of joystick to controller mappings
	SDL_GameControllerAddMappingsFromFile("./assets/gamecontrollerdb.txt");

	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
}

Input::~Input() {
	auto kart_iter = m_players.begin();
	while (kart_iter != m_players.end()) {
		auto kart_id = kart_iter->first;
		kart_iter++;
		ForgetPlayer(kart_id);
	}
	auto js_iter = m_joysticks.begin();
	while (js_iter != m_joysticks.end()) {
		auto js_id = js_iter->first;
		js_iter++;
		JoystickRemoved(js_id);
	}
}

void Input::setup() {
	lastKbInput = NEWEVENT(Input);
}

void Input::HandleEvents() {

	std::map<entity_id, bool> seen_karts;
	std::vector<entity_id> seen_kart_ids;
	Events::StartMenuInputEvent *menu = NULL;

	// TODO: Get events which would cause haptic rumple
	// Get Event Instructions
	for ( Events::Event *mail_event : (m_Mailbox.checkMail()) )
	{
		switch ( mail_event->type )
		{
		case Events::EventType::PlayerKart:
			{
				// What kart is this input for?
				auto kart_id = ((Events::PlayerKartEvent *)mail_event)->kart_id;
				seen_karts[kart_id] = true;
				seen_kart_ids.push_back(kart_id);
				break;
			}
		case Events::EventType::KartDestroyed:
		case Events::EventType::AiKart:
			{
				auto kart_id = ((Events::KartDestroyedEvent *)mail_event)->kart_id;
				if (m_players.count(kart_id))
					ForgetPlayer(kart_id);
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
	memcpy(kbInput, lastKbInput, sizeof(*kbInput));
	kbInput->id = event_id;
	// Reset Development Tools
	kbInput->print_position = false;
	kbInput->reset_requested = false;
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
			outEvents.push_back( NEWEVENT( Quit ) );
			break;
		case SDL_KEYDOWN:
			OnKeyDown(&sdl_event, &outEvents, kbInput);
			break;
		case SDL_KEYUP:
			OnKeyUp(&sdl_event, kbInput);
			break;
		case SDL_JOYDEVICEADDED:
			JoystickAdded(sdl_event.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			{
				auto inst_id = (SDL_JoystickID)sdl_event.jdevice.which;
				JoystickRemoved(inst_id);
				m_joysticks.erase(inst_id);
			}
			break;
		default:
			//DEBUGOUT("SDL Event type:%i\n", sdl_event->type);
			break;
		}
	}
	// Save keyboard input
	memcpy(lastKbInput, kbInput, sizeof(*kbInput));

	// Give Menu controls
	// If no players active poll all
	// controllers
	if (menu) {
		if (seen_kart_ids.size() == 0) {
			for (auto joy : m_free_joysticks) {
				if (joy->controller != NULL) {
					PollController(joy->controller, &outEvents, kbInput);
				} else {
					PollJoystick(joy->joystick, &outEvents, kbInput);
				}
			}
		}
		menu->aPressed = kbInput->aPressed;
		menu->bPressed = kbInput->bPressed;
		menu->xPressed = kbInput->xPressed;
		menu->yPressed = kbInput->yPressed;
		menu->onePressed = kbInput->onePressed;
		menu->twoPressed = kbInput->twoPressed;
		menu->threePressed = kbInput->threePressed;
		menu->fourPressed = kbInput->fourPressed;
	}

	// Any players leave?
	auto kart_iter = m_players.begin();
	while (kart_iter != m_players.end()) {
		auto kart_id = kart_iter->first;
		kart_iter++;
		if (seen_karts.count(kart_id) == 0) {
			ForgetPlayer(kart_id);
		}
	}


	// Gen local player data for active players
	// Note: First players get controllers
	for (auto kart_id : seen_kart_ids) {
		player_kart *kart_local = NULL;

		// Have we seen kart before?
		bool first_sighting = m_players.count(kart_id) == 0;
		if (first_sighting) {
			kart_local = new player_kart();

			kart_local->j= NULL;

			m_players[kart_id] = kart_local;
		} else {
			kart_local = m_players[kart_id];
		}

		// Is kart matched to joystick yet?
		if (kart_local->j == NULL) {
			// Get next free joysick
			if (m_free_joysticks.size() > 0) {
				kart_local->j = *m_free_joysticks.begin();
				m_free_joysticks.erase(m_free_joysticks.begin());
				auto j_id = kart_local->j->inst_id;
				m_taken_joysticks[j_id] = kart_id;
			} else if (first_sighting) {
				DEBUGOUT("Warning: Could not get joystick for player of kart %d\n", kart_id);
			}
		}
	}


	// Gen input for active players
	// Note: Last player gets keyboard input
	bool kbInputTaken = false;
	for (int i = seen_kart_ids.size() - 1; i >= 0; i--) {
		auto kart_id = seen_kart_ids[i];
		auto kart_local = m_players[kart_id];

		// Input to be sent
		Events::InputEvent *input = NULL;
		if (!kbInputTaken) {
			input = kbInput;
			kbInputTaken = true;
		} else {
			input = NEWEVENT(Input);
		}
		input->kart_id = kart_id;

		if (kart_local->j) {
			if (kart_local->j->controller) {
				PollController(kart_local->j->controller, &outEvents, input);
			} else if (kart_local->j->joystick) {
				PollJoystick(kart_local->j->joystick, &outEvents, input);
			}
		}


		if (menu) {
			menu->aPressed |= input->aPressed;
			menu->bPressed |= input->bPressed;
			menu->xPressed |= input->xPressed;
			menu->yPressed |= input->yPressed;
		}

		// Send input
		outEvents.push_back(input);
		//DEBUGOUT("id: %d, r: %f, a: %d\n", kart_id, input->rightTrigger, input->aPressed);
	}

	m_Mailbox.emptyMail();
	
	if (menu)
		outEvents.push_back(menu);
	m_Mailbox.sendMail(outEvents);
}

#define KEY(x) \
	break; \
	case SDLK_ ## x : 

void Input::OnKeyDown(SDL_Event *event, std::vector<Events::Event *> *eventQueue, Events::InputEvent *outInput){
	SDL_Keycode keycode = event->key.keysym.sym;

	// Handle key press for sending InputEvent to physics
	switch (keycode)
	{
	KEY(ESCAPE)	//This should pause the game but for now it just exits the program
		eventQueue->push_back( NEWEVENT( Quit ) );
	KEY(a)
		outInput->leftThumbStickRL = -1;
	KEY(d)
		outInput->leftThumbStickRL = 1;
	KEY(RIGHT)
		outInput->leftThumbStickRL = 1;
	KEY(LEFT)
		outInput->leftThumbStickRL = -1;
	KEY(w)
		outInput->rightTrigger = 1;
	KEY(s)
		outInput->leftTrigger = 1;
	KEY(SPACE)
		outInput->bPressed = true;
	KEY(LSHIFT)
		outInput->aPressed = true;
	KEY(RSHIFT)
		outInput->xPressed = true;
	KEY(1)
		outInput->onePressed = true;
	KEY(2)
		outInput->twoPressed = true;
	KEY(3)
		outInput->threePressed = true;
	KEY(4)
		outInput->fourPressed = true;

		// Development Tools
	KEY(z)
		outInput->print_position = true;
	KEY(r)
		outInput->reset_requested = true;
	default:
		break;
	}
}

void Input::OnKeyUp(SDL_Event *event, Events::InputEvent *outInput) {
	SDL_Keycode keycode = event->key.keysym.sym;

	switch (keycode)
	{
	KEY(a)
		outInput->leftThumbStickRL = 0;
	KEY(d)
		outInput->leftThumbStickRL = 0;
	KEY(RIGHT)
		outInput->leftThumbStickRL = 0;
	KEY(LEFT)
		outInput->leftThumbStickRL = 0;
	KEY(w)
		outInput->rightTrigger = 0;
	KEY(s)
		outInput->leftTrigger = 0;
	KEY(SPACE)
		outInput->bPressed = false;
	KEY(LSHIFT)
		outInput->aPressed = false;
	KEY(RSHIFT)
		outInput->xPressed = false;
	KEY(1)
		outInput->onePressed = false;
	KEY(2)
		outInput->twoPressed = false;
	KEY(3)
		outInput->threePressed = false;
	KEY(4)
		outInput->fourPressed = false;
	default:
		break;
	}
}
#undef KEY


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

void Input::JoystickAdded(int device_id) {
	auto joy = SDL_JoystickOpen(device_id);
	if (joy == NULL) {
		return;
	}

	struct joystick *js = new joystick();

	js->joystick = joy;
	js->inst_id = SDL_JoystickInstanceID(js->joystick);
	js->haptic = SDL_HapticOpenFromJoystick(js->joystick);
	if (SDL_IsGameController(device_id)) {
		js->controller = SDL_GameControllerOpen(device_id);
		DEBUGOUT("Found controller %d: %s\n", device_id,
				SDL_GameControllerName(js->controller));
	} else {
		js->controller = NULL;
	}

	m_joysticks[js->inst_id] = js;
	m_free_joysticks.push_back(js);
}

void Input::JoystickRemoved(SDL_JoystickID inst_id) {
	struct joystick *js = NULL;
	if (m_taken_joysticks.count(inst_id) != 0) {
	
		auto kart_id = m_taken_joysticks[inst_id];
		js = ForgetPlayer(kart_id);
		m_players.erase(kart_id);
		m_taken_joysticks.erase(inst_id);
	} else {
		for (size_t i = 0; i < m_free_joysticks.size(); i++) {
			auto js_candidate = m_free_joysticks[i];
			if (js_candidate->inst_id == inst_id) { 
				js = m_free_joysticks[i];
				m_free_joysticks.erase(m_free_joysticks.begin() + i);
				break;
			}
		}
	}

	if (js == NULL)
		return;

	if (js->controller)
		SDL_GameControllerClose(js->controller);
	if (js->haptic)
		SDL_HapticClose(js->haptic);
	if (js->joystick)
		SDL_JoystickClose(js->joystick);

	delete js;
}

Input::joystick *Input::ForgetPlayer(entity_id kart_id) {

	if (m_players.count(kart_id) == 0) {
		return NULL;
	}

	auto kart = m_players[kart_id];
	auto joystick = kart->j;

	m_players.erase(kart_id);
	delete kart;

	m_free_joysticks.insert(m_free_joysticks.begin(), joystick);

	return joystick;
}
