#include <fmod.h>
#include <fmod_errors.h>

#include "audio.h"

#define ERRCHECK(res)	if (res != FMOD_OK){\
	DEBUGOUT("FMOD error! (%d) %s\n", res, FMOD_ErrorString(res));\
    exit(-1);}

#define PITCHSCALE 0.1f
#define MAX_PITCH 2.0f
#define ENGINE_SOUND_FILE "assets/audio/engineNoise3.wav"

#define DOPPLER_SCALE 1.0f
#define DISTANCE_FACTOR 1.0f
#define ROLL_OFF_SCALE 1.0f

#define MUSIC_VOLUME 0.03f
#define SOUND_EFFECTS_VOLUME 0.1f
#define LOW_ENGINE_NOISE_VOLUME 1.0f

Audio::Audio() {

	m_pMailbox = new Events::Mailbox();	
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::KartCreated );
	m_pMailbox->request( Events::EventType::KartDestroyed );
	m_pMailbox->request( Events::EventType::PlayerKart );
	m_pMailbox->request( Events::EventType::AiKart );
	m_pMailbox->request( Events::EventType::AudioPlayPause);
	m_pMailbox->request( Events::EventType::PowerupPickup );
}

Audio::~Audio() {

	m_system->release();

	if(m_pMailbox)
		delete m_pMailbox;
}

void Audio::setup() {
	SetupHardware();

	//// Setup music and sound effects channels
	ERRCHECK(m_system->createChannelGroup("Effects", &m_channelGroupEffects));
	ERRCHECK(m_system->createChannelGroup("Music", &m_channelGroupMusic));

	musicVol = MUSIC_VOLUME;
	sfxVol = SOUND_EFFECTS_VOLUME;

	m_channelGroupMusic->setVolume(musicVol);
	m_channelGroupEffects->setVolume(sfxVol);

	// Load all the sound files
	LoadMusic("assets/audio/BrainDead.mp3");
	playMusic = true;

	Sounds.PowerUp = LoadSound("assets/audio/powerup2.wav");
	Sounds.LowFreqEngine = LoadSound("assets/audio/engineIdleNoise1.wav");
	Sounds.MachineGun = LoadSound("assets/audio/machineGun1.aiff");

	StartMusic();
	//Setup3DEnvironment();
}

int Audio::SetupHardware(){

	FMOD_RESULT result;
	unsigned int version;
	int numdrivers;
	FMOD_SPEAKERMODE speakermode;
	FMOD_CAPS caps;
	char name[256];
	/*
	Create a System object and initialize.
	*/
	result = FMOD::System_Create(&m_system);
	ERRCHECK(result);
	result = m_system->getVersion(&version);
	ERRCHECK(result);
	if (version < FMOD_VERSION)
	{
		printf("Error! You are using an old version of FMOD %08x.  This program requires %08x\n", 
			version, FMOD_VERSION);
		return 0;
	}
	result = m_system->getNumDrivers(&numdrivers);
	ERRCHECK(result);
	if (numdrivers == 0)
	{
		result = m_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
		ERRCHECK(result);
	}
	else
	{
		result = m_system->getDriverCaps(0, &caps, 0, &speakermode);
		ERRCHECK(result);
		/*
		Set the user selected speaker mode.
		*/
		result = m_system->setSpeakerMode(speakermode);
		ERRCHECK(result);
		if (caps & FMOD_CAPS_HARDWARE_EMULATED)
		{
			/*
			The user has the 'Acceleration' slider set to off! This is really bad 
			for latency! You might want to warn the user about this.
			*/
			result = m_system->setDSPBufferSize(1024, 10);
			ERRCHECK(result);
		}
		result = m_system->getDriverInfo(0, name, 256, 0);
		ERRCHECK(result);
		if (strstr(name, "SigmaTel"))
		{
			/*
			Sigmatel sound devices crackle for some reason if the format is PCM 16bit.
			PCM floating point output seems to solve it.
			*/
			result = m_system->setSoftwareFormat(48000, FMOD_SOUND_FORMAT_PCMFLOAT, 0,0, 
				FMOD_DSP_RESAMPLER_LINEAR);
			ERRCHECK(result);
		}
	}
	result = m_system->setSoftwareChannels(100);
	ERRCHECK(result);
	result = m_system->setHardwareChannels(64);
	result = m_system->init(100, FMOD_INIT_NORMAL, 0);
	if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)
	{
		/*
		Ok, the speaker mode selected isn't supported by this soundcard. Switch it 
		back to stereo...
		*/
		result = m_system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
		ERRCHECK(result);
		/*
		... and re-init.
		*/
		result = m_system->init(100, FMOD_INIT_NORMAL, 0);
	}
	return 1;
}

void Audio::SetupEngineSounds(struct kart_audio *kart_local){

	FMOD::Sound *engineSound;
	FMOD::Channel *engineNoisechannel;
	FMOD::Channel *idelNoiseChannel;
	FMOD::DSP *dsp;

	ERRCHECK(m_system->createSound(ENGINE_SOUND_FILE, FMOD_3D, 0, &engineSound));

	ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, engineSound, true, &engineNoisechannel));
	ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.LowFreqEngine], true, &idelNoiseChannel));

	ERRCHECK(m_system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &dsp));
	ERRCHECK(engineNoisechannel->addDSP(dsp, 0));
	ERRCHECK(idelNoiseChannel->addDSP(dsp, 0));

	auto kart_entity = GETENTITY(kart_local->kart_id, CarEntity);
	FMOD_VECTOR *pos = new FMOD_VECTOR();
	pos->x = kart_entity->Pos.x;
	pos->y = kart_entity->Pos.y;
	pos->z = kart_entity->Pos.z;

	ERRCHECK(engineNoisechannel->set3DAttributes(pos, 0));
	ERRCHECK(idelNoiseChannel->set3DAttributes(pos, 0));
	ERRCHECK(engineNoisechannel->setMode(FMOD_LOOP_NORMAL));
	ERRCHECK(idelNoiseChannel->setMode(FMOD_LOOP_NORMAL));

	idelNoiseChannel->setVolume(LOW_ENGINE_NOISE_VOLUME);

	kart_local->enginePitch = 0;
	kart_local->engineSound = engineSound;
	kart_local->engineChannel = engineNoisechannel;
	kart_local->idleNoiseChannel = idelNoiseChannel;
	kart_local->kartEngineDSP = dsp;

	ERRCHECK(engineNoisechannel->setChannelGroup(m_channelGroupEngineSound));
	ERRCHECK(idelNoiseChannel->setChannelGroup(m_channelGroupEngineSound));

	ERRCHECK(engineNoisechannel->setPaused(false));
	ERRCHECK(idelNoiseChannel->setPaused(false));
}

void Audio::DestroyEngineSounds(struct kart_audio *kart) {

	// @Kyle: How do you get fmod to release things like the engineSound, or
	// engineNoiseChannel?
	// delete kart->idleNoiseChannel;
	// delete kart->engineChannel;
	// delete kart->soundsChannel;
	// deltet kart->engineDSP;
}

void Audio::Setup3DEnvironment() {
	ERRCHECK(m_system->set3DSettings(DOPPLER_SCALE, DISTANCE_FACTOR, ROLL_OFF_SCALE));
}

void Audio::OutputMemUsage(){
	float stream;
	float update;
	float total;
	uint32_t memUsage;

	m_system->getCPUUsage(0, &stream, 0, &update, &total);
	m_system->getMemoryInfo(FMOD_MEMBITS_ALL, FMOD_EVENT_MEMBITS_ALL, &memUsage, 0);

	DEBUGOUT("Audio CPU usage:  Stream: %lf | Update: %lf | Total: %lf\n", stream, update, total);
	DEBUGOUT("Audio MEM usage:  %d\n", memUsage);
}

int Audio::LoadMusic(char* file){
	FMOD_RESULT result;
	FMOD::Sound *newMusic;
	
	result = m_system->createStream(file, FMOD_DEFAULT, 0, &newMusic);
	ERRCHECK(result);

	m_MusicList.push_back(newMusic);

	return m_MusicList.size() - 1;
}

int Audio::LoadSound(char* file){
	FMOD::Sound *newSound;
	FMOD::Channel *newChannel;

	ERRCHECK(m_system->createSound(file, FMOD_3D, 0, &newSound));

	m_SoundList.push_back(newSound);
	m_SoundsChannelList.push_back(newChannel);

	return m_SoundList.size() - 1;
}

void Audio::StartMusic(){
	FMOD::Channel *musicChannel;
	m_system->playSound(FMOD_CHANNEL_FREE, m_MusicList[0], 0, &musicChannel);
	musicChannel->setChannelGroup(m_channelGroupMusic);
	m_channelGroupMusic->setVolume(MUSIC_VOLUME);
}

void Audio::ToggleMusic(){
	playMusic = !playMusic;
	m_channelGroupMusic->setPaused(playMusic);
}

void Audio::UpdateListenerPos(){

	FMOD_VECTOR position;
	FMOD_VECTOR velocity;
	FMOD_VECTOR forward;
	FMOD_VECTOR up;
	float speed = 0;

	auto kart_entity = GETENTITY(primary_player, CarEntity);
	if (kart_entity != NULL) {
		position.x = kart_entity->Pos.x;
		position.y = kart_entity->Pos.y;
		position.z = kart_entity->Pos.z;

		// Get the Kart's forward vector and remove the y component and then normalize to get unit vector length
		btVector3 temp = kart_entity->forDirection;
		temp.setY(0);
		temp.normalize();

		speed = kart_entity->Speed;
		velocity.x = kart_entity->forDirection.x() * speed;
		velocity.y = kart_entity->forDirection.y() * speed;
		velocity.z = kart_entity->forDirection.z() * speed;

		forward.x = temp.getX();
		forward.y = temp.getY();
		forward.z = temp.getZ();
	} else {
		// No primary player yet or
		// primary player is gone so play
		// sound from center of world.
		// Might be used in menus.
		// @Kyle: Does FMOD_VECTOR self init to zero?
		forward.x = 0;
		forward.y = 0;
		forward.z = 0;
		position = velocity = forward;
	}

	up.x = 0;
	up.y = 1;
	up.z = 0;

	ERRCHECK(m_system->set3DListenerAttributes(0, &position, &velocity, &forward, &up));
}

void Audio::UpdateKartsPos(struct kart_audio *kart_local){
	auto kart_entity = GETENTITY(kart_local->kart_id, CarEntity);

	FMOD_VECTOR *pos = new FMOD_VECTOR();
	pos->x = kart_entity->Pos.x;
	pos->y = kart_entity->Pos.y;
	pos->z = kart_entity->Pos.z;
	ERRCHECK(kart_local->engineChannel->set3DAttributes(pos, 0));
	ERRCHECK(kart_local->idleNoiseChannel->set3DAttributes(pos, 0));
}

void Audio::update(Real seconds){

	for( Events::Event *event : m_pMailbox->checkMail() )
	{
		switch (event->type) {
		case Events::EventType::AudioPlayPause:
		{
			ToggleMusic();
		}
		break;
		case Events::EventType::KartCreated:
		{
			entity_id kart_id = ((Events::KartCreatedEvent *)events)->kart_id;
			struct kart_audio *kart = new kart_audio();

			kart->kart_id = kart_id;
			SetupEngineSounds(kart);
			m_karts[kart_id] = kart;
		}
		break;
		case Events::EventType::KartDestroyed:
		{
			entity_id kart_id = ((Events::KartDestroyedEvent *)events)->kart_id;
			DestroyEngineSounds(kart);
			m_karts.erase(kart_id);
		}
		break;
		case Events::EventType::PlayerKart:
		{
			entity_id kart_id = ((Events::KartDestroyedEvent *)events)->kart_id;
			if (primary_player == 0) {
				primary_player = kart_id;
			}
			UpdateKartsPos(kart_id);
		}
		break;
		case Events::EventType::AiKart:
		{
			entity_id kart_id = ((Events::KartDestroyedEvent *)events)->kart_id;
			UpdateKartsPos(kart_id);
		}
		break;
		case Events::EventType::Input:
		{
			// Handle all one-off sound effects
			Events::InputEvent *input = (Events::InputEvent *)events;
			entity_id kart_id = events->kart_id;
			auto kart_local = m_karts[kart_id];
			auto kart_entity = GETENTITY(kart_id, CarEntity);

			if(input->aPressed)  // Weapon fired
			{
				FMOD_VECTOR pos;
				pos.x = kart_entity->Pos.x;
				pos.y = kart_entity->Pos.y;
				pos.z = kart_entity->Pos.z;

				bool isPlaying = false;
				kart_local->soundsChannel->isPlaying(&isPlaying);

				if(!isPlaying)
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.MachineGun],
								true, &(kart_local->soundsChannel)));
					kart_local->soundsChannel->set3DAttributes(&pos, 0);
					kart_local->soundsChannel->setChannelGroup(m_channelGroupEffects);
					kart_local->soundsChannel->setPaused(false);
				}
			}
			if(input->bPressed)
			{
				// Play a sound effect
				int x = 0;
			}

			Real lerpAmt = seconds * 2.0f;
			Real newPitch = Lerp(kart_local->enginePitch, input->rightTrigger * MAX_PITCH, lerpAmt);

			kart_local->enginePitch = newPitch;

			// Update Kart Engine Sounds
			ERRCHECK(kart_local->engineDSP->setParameter(FMOD_DSP_PITCHSHIFT_PITCH, newPitch));
		}
		break;
		case Events::EventType::PowerupPickup:
		{
			/* @Kyle: Sorry I don't have powerups refactored.
			 * Eric has suggested some ideas for my work on
			 * the powerups which I want to use.
			 * The changes should not affect this
			 * piece of code. This code should work once
			 * powerups are changed.
			 * 
			 * Would you be willing to leave this commented out until
			 * I've worked on powerups
			Events::PowerupPickupEvent *pickup = (Events::PowerupPickupEvent *)event;
			FMOD_VECTOR pos;

			Vector3 power_pos = pickup->pos;
			pos.x = power_pos.x;
			pos.y = power_pos.y;
			pos.z = power_pos.z;

			FMOD::Channel *channel;

			int PowerUp = Sounds.PowerUp;

			ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[PowerUp], true, &channel));
			channel->setChannelGroup(m_channelGroupEffects);
			channel->set3DAttributes(&pos, 0);
			channel->setPaused(false);
			*/
		}
		break;
		}
	}
	m_pMailbox->emptyMail();

	UpdateListenerPos();
	ERRCHECK(m_system->update());

	//OutputMemUsage();
}


