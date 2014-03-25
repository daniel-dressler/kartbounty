#include <fmod.h>
#include <fmod_errors.h>

#include "audio.h"

#define ERRCHECK(res)	if (res != FMOD_OK){\
	DEBUGOUT("FMOD error! (%d) %s\n", res, FMOD_ErrorString(res));\
    exit(-1);}

#define PITCHSCALE 0.1f
#define MAX_PITCH 2.0f
#define DOPPLER_SCALE 1.0f
#define DISTANCE_FACTOR 1.0f
#define ROLL_OFF_SCALE 1.0f

// Volumes
#define MUSIC_VOLUME 0.03f
#define SOUND_EFFECTS_VOLUME 0.5f
#define LOW_ENGINE_NOISE_VOLUME 0.2f

#define MIN_HANDBRAKE_SPEED 3.0			// Min speed for handbrake audio to be played
#define MAX_COLLISION_FORCE 5000		// Max expected collision force for scaling from [0,1] for volume

Audio::Audio() {

	m_pMailbox = new Events::Mailbox();	
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::KartCreated );
	m_pMailbox->request( Events::EventType::KartDestroyed );
	m_pMailbox->request( Events::EventType::PlayerKart );
	m_pMailbox->request( Events::EventType::AiKart );
	m_pMailbox->request( Events::EventType::AudioPlayPause);
	m_pMailbox->request( Events::EventType::PowerupPickup );
	m_pMailbox->request( Events::EventType::PowerupActivated );
	m_pMailbox->request( Events::EventType::KartHitByBullet );
	m_pMailbox->request( Events::EventType::KartColideArena );
	m_pMailbox->request( Events::EventType::KartColideKart );
	m_pMailbox->request( Events::EventType::KartHandbrake );
	m_pMailbox->request( Events::EventType::TogglePauseGame );
	m_pMailbox->request( Events::EventType::RoundStart );
	m_pMailbox->request( Events::EventType::RoundEnd );
	m_pMailbox->request( Events::EventType::Reset );
	primary_player = 0;
}

Audio::~Audio() {

	for(int i = 0; i < m_SoundList.size(); i++)
	{
		m_SoundList[i]->release();
	}

	m_system->release();

	if(m_pMailbox)
		delete m_pMailbox;
}

void Audio::setup() {
	SetupHardware();

	gamePaused = false;

	//// Setup music and sound effects channels
	ERRCHECK(m_system->createChannelGroup("Effects", &m_channelGroupEffects));
	ERRCHECK(m_system->createChannelGroup("EngineNoise", &m_channelGroupEngineSound));
	ERRCHECK(m_system->createChannelGroup("Music", &m_channelGroupMusic));

	musicVol = MUSIC_VOLUME;
	sfxVol = SOUND_EFFECTS_VOLUME;

	m_channelGroupMusic->setVolume(musicVol);
	m_channelGroupEffects->setVolume(sfxVol);

	// Load all the sound files
	LoadMusic("assets/audio/BrainDead.mp3");
	playMusic = true;

	Sounds.EngineSound = LoadSound("assets/audio/engineNoise3.wav", FMOD_3D);
	Sounds.PowerUpPickUp = LoadSound("assets/audio/powerup1.wav", FMOD_3D);
	Sounds.SpeedPowerup = LoadSound("assets/audio/powerup2.wav", FMOD_3D);
	Sounds.GoldChestPowerup = LoadSound("assets/audio/goldChestPickup.mp3", FMOD_3D);
	Sounds.HealthPowerup = LoadSound("assets/audio/healthpowerup.mp3", FMOD_3D);
	Sounds.LowFreqEngine = LoadSound("assets/audio/engineIdleNoise1.wav", FMOD_3D);
	Sounds.MachineGun = LoadSound("assets/audio/machineGun1.aiff", FMOD_3D);
	Sounds.WallCollision = LoadSound("assets/audio/kartCollision1.wav", FMOD_3D);
	Sounds.Skid = LoadSound("assets/audio/skid1.wav", FMOD_3D);
	Sounds.KartExplode = LoadSound("assets/audio/KartExplosion.mp3", FMOD_3D);
	Sounds.RoundStart = LoadSound("assets/audio/roundstart1.mp3", FMOD_2D);
	Sounds.Boo = LoadSound("assets/audio/boo.mp3", FMOD_2D);
	Sounds.Cheer = LoadSound("assets/audio/cheer.mp3", FMOD_2D);
	Sounds.KartBulletHit = LoadSound("assets/audio/KartBulletHit.mp3", FMOD_3D);

	//StartMusic();
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

	FMOD::Channel *engineNoisechannel;
	FMOD::Channel *idelNoiseChannel;
	FMOD::DSP *dsp;

	ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.EngineSound], true, &engineNoisechannel));
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

	kart_local->enginePitch = 1;
	kart_local->engineSound = m_SoundList[Sounds.EngineSound];
	kart_local->engineChannel = engineNoisechannel;
	kart_local->idleNoiseChannel = idelNoiseChannel;
	kart_local->engineDSP = dsp;

	ERRCHECK(engineNoisechannel->setChannelGroup(m_channelGroupEngineSound));
	ERRCHECK(idelNoiseChannel->setChannelGroup(m_channelGroupEngineSound));

	ERRCHECK(engineNoisechannel->setPaused(false));
	ERRCHECK(idelNoiseChannel->setPaused(false));

	delete pos;
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

int Audio::LoadSound(char* file, FMOD_MODE mode){
	FMOD::Sound *newSound;
	FMOD::Channel *newChannel;

	DEBUGOUT("Loading file %s\n", file);
	
	ERRCHECK(m_system->createSound(file, mode, 0, &newSound));

	m_SoundList.push_back(newSound);
	m_SoundChannelList.push_back(newChannel);

	return m_SoundList.size() - 1;
}

void Audio::StartMusic(){
	FMOD::Channel *musicChannel;
	m_system->playSound(FMOD_CHANNEL_FREE, m_MusicList[0], 0, &musicChannel);
	musicChannel->setMode(FMOD_LOOP_NORMAL);
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
		forward.x = 1;
		forward.y = 0;
		forward.z = 0;
		position = velocity = forward;
	}

	up.x = 0;
	up.y = 1;
	up.z = 0;

	ERRCHECK(m_system->set3DListenerAttributes(0, &position, &velocity, &forward, &up));
}

void Audio::UpdateKartsPos(entity_id kart_id)
{

	auto kart_entity = GETENTITY(kart_id, CarEntity);
	auto kart = m_karts[kart_id];

	if (kart != NULL)
	{
		FMOD_VECTOR *pos = new FMOD_VECTOR();
		pos->x = kart_entity->Pos.x;
		pos->y = kart_entity->Pos.y;
		pos->z = kart_entity->Pos.z;
		ERRCHECK(kart->engineChannel->set3DAttributes(pos, 0));
		ERRCHECK(kart->idleNoiseChannel->set3DAttributes(pos, 0));

		delete pos;
	}
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
		case Events::EventType::TogglePauseGame:
			{
				gamePaused = !gamePaused;
				m_channelGroupEngineSound->setPaused(gamePaused);
			}
			break;
		case Events::EventType::RoundStart:
			{
				// Should add code in here to play countdown
				ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.RoundStart],
					false, &roundStartChannel));

				m_channelGroupEngineSound->setPaused(false);
			}
			break;
		case Events::EventType::RoundEnd:
			{
				Events::RoundEndEvent *roundEnd = (Events::RoundEndEvent *)event;
				if(roundEnd->playerWon)
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.Cheer],
						false, &roundStartChannel));
				}
				else
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.Boo],
						false, &roundStartChannel));
				}				
			}
			break;
		case Events::EventType::KartCreated:
			{
				entity_id kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
				auto *kart = new Audio::kart_audio();

				kart->kart_id = kart_id;
				SetupEngineSounds(kart);
				m_karts[kart_id] = kart;
			}
			break;
		case Events::EventType::KartDestroyed:
			{
				entity_id kart_id = ((Events::KartDestroyedEvent *)event)->kart_id;
				DestroyEngineSounds(m_karts[kart_id]);
				m_karts.erase(kart_id);
			}
			break;
		case Events::EventType::PlayerKart:
			{
				entity_id kart_id = ((Events::PlayerKartEvent *)event)->kart_id;
				if (primary_player == 0) {
					primary_player = kart_id;
				}
				UpdateKartsPos(kart_id);
			}
			break;
		case Events::EventType::AiKart:
			{
				entity_id kart_id = ((Events::AiKartEvent *)event)->kart_id;
				UpdateKartsPos(kart_id);
			}
			break;
		case Events::EventType::KartColideKart:
			{
			Events::KartColideKartEvent * collisionEvent = (Events::KartColideKartEvent *)event;
				auto kart_local = m_karts[collisionEvent->kart_id];

				if(kart_local == NULL)	// In case we get this event for a kart that was already deleted
					break;

				auto kart_entity = GETENTITY(collisionEvent->kart_id, CarEntity);

				FMOD_VECTOR pos;
				pos.x = kart_entity->Pos.x;
				pos.y = kart_entity->Pos.y;
				pos.z = kart_entity->Pos.z;

				bool isPlaying = false;
				kart_local->collisionChannel->isPlaying(&isPlaying);

				if(!isPlaying)
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.WallCollision],
						true, &kart_local->collisionChannel));
					kart_local->collisionChannel->set3DAttributes(&pos, 0);
					float volume = Clamp(collisionEvent->force / MAX_COLLISION_FORCE);
					kart_local->collisionChannel->setVolume(volume);
					kart_local->collisionChannel->setChannelGroup(m_channelGroupEffects);
					kart_local->collisionChannel->setPaused(false);
				}
				//DEBUGOUT("Kart Colide Kart event with force: %f\n", collisionEvent->force);
			}
			break;
		case Events::EventType::KartColideArena:
			{
				Events::KartColideArenaEvent * collisionEvent = (Events::KartColideArenaEvent *)event;
				auto kart_local = m_karts[collisionEvent->kart_id];
				
				if(kart_local == NULL)
					break;

				auto kart_entity = GETENTITY(collisionEvent->kart_id, CarEntity);

				FMOD_VECTOR pos;
				pos.x = kart_entity->Pos.x;
				pos.y = kart_entity->Pos.y;
				pos.z = kart_entity->Pos.z;

				bool isPlaying = false;
				kart_local->collisionChannel->isPlaying(&isPlaying);

				if(!isPlaying)
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.WallCollision],
						true, &kart_local->collisionChannel));
					kart_local->collisionChannel->set3DAttributes(&pos, 0);
					float volume = Clamp(collisionEvent->force / MAX_COLLISION_FORCE);
					kart_local->collisionChannel->setVolume(volume);
					kart_local->collisionChannel->setChannelGroup(m_channelGroupEffects);
					kart_local->collisionChannel->setPaused(false);
				}
				//DEBUGOUT("Kart Colide Arena event with force: %f\n", collisionEvent->force);
			}
			break;
		case Events::EventType::KartHitByBullet:
			{
				Events::KartHitByBulletEvent *impact = (Events::KartHitByBulletEvent *)event;
				auto kart_local = m_karts[impact->kart_id];
				
				if(kart_local == NULL)
					break;

				auto kart_entity = GETENTITY(impact->kart_id, CarEntity);
				if(kart_entity == NULL)
					break;

				FMOD_VECTOR pos;
				pos.x = kart_entity->Pos.x;
				pos.y = kart_entity->Pos.y;
				pos.z = kart_entity->Pos.z;

				bool isPlaying = false;
				kart_local->bulletHitChannel->isPlaying(&isPlaying);

				if(!isPlaying)
				{
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.KartBulletHit],
						true, &kart_local->bulletHitChannel));
					kart_local->bulletHitChannel->set3DAttributes(&pos, 0);
					kart_local->bulletHitChannel->setChannelGroup(m_channelGroupEffects);
					kart_local->bulletHitChannel->setPaused(false);
				}
			}
			break;
		case Events::EventType::Input:
			{
				if(gamePaused)
					break;

				// Handle all one-off sound effects
				Events::InputEvent *input = (Events::InputEvent *)event;
				entity_id kart_id = input->kart_id;
				auto kart_local = m_karts[kart_id];

				if(input->aPressed)  // Weapon fired
				if (kart_local != NULL)
				{
					auto kart_entity = GETENTITY(kart_id, CarEntity);

					if(gamePaused)
						break;
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

					Real clampedSeconds = Clamp(seconds, 0.0f, 0.10f);		// This is incase frame rate really drops.

					float lerpAmt = clampedSeconds * 2.0f;
					float newPitch = Lerp(kart_local->enginePitch, input->rightTrigger * MAX_PITCH, lerpAmt);
					newPitch = Clamp(newPitch, 0.0f, 2.0f);

					newPitch = Clamp(newPitch, 0.0f, 2.0f);

					kart_local->enginePitch = newPitch;

					if(newPitch > 2.0f || newPitch < 0.0f)
					{
						DEBUGOUT("WTF?\n");
					}
					// Update Kart Engine Sounds
					ERRCHECK(kart_local->engineDSP->setParameter(FMOD_DSP_PITCHSHIFT_PITCH, newPitch));
				}
			}
			break;
		case Events::EventType::KartHandbrake:
			{
				if(gamePaused)
					break;
				auto handbrakeEvent = (Events::KartHandbrakeEvent *)event;
				auto kart_local = m_karts[handbrakeEvent->kart_id];

				if (kart_local != NULL)
				{
					FMOD_VECTOR pos;
					pos.x = handbrakeEvent->pos.x;
					pos.y = handbrakeEvent->pos.y;
					pos.z = handbrakeEvent->pos.z;

					bool isPlaying = false;
					kart_local->soundsChannel->isPlaying(&isPlaying);
				
					if(!isPlaying && (fabs(handbrakeEvent->speed) > MIN_HANDBRAKE_SPEED))
					{
						ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.Skid],
							true, &(kart_local->soundsChannel)));
						kart_local->soundsChannel->set3DAttributes(&pos, 0);
						kart_local->soundsChannel->setChannelGroup(m_channelGroupEffects);
						kart_local->soundsChannel->setPaused(false);
					}
				}
			}
			break;
		case Events::EventType::PowerupPickup:
			{
				Events::PowerupPickupEvent *pickup = (Events::PowerupPickupEvent *)event;
				FMOD_VECTOR pos;

				Vector3 power_pos = pickup->pos;
				pos.x = power_pos.x;
				pos.y = power_pos.y;
				pos.z = power_pos.z;

				FMOD::Channel *channel;
				switch (pickup->powerup_type)
				{
				case Entities::GoldCasePowerup:
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.GoldChestPowerup], true, &channel));
					break;
				case Entities::HealthPowerup:
					{
						ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.HealthPowerup], true, &channel));
					}
					break;
				default:
					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.PowerUpPickUp], true, &channel));
					break;
				}

				channel->setChannelGroup(m_channelGroupEffects);
				channel->set3DAttributes(&pos, 0);
				channel->setPaused(false);
			}
			break;
		case Events::EventType::PowerupActivated:
			{
				Events::PowerupActivatedEvent *powUsed = (Events::PowerupActivatedEvent *)event;

				// Get where the sound event happened. 
				FMOD_VECTOR pos;
				pos.x = powUsed->pos.x;
				pos.y = powUsed->pos.y;
				pos.z = powUsed->pos.z;

				FMOD::Channel *channel;

				switch (powUsed->powerup_type)
				{
				case Entities::SpeedPowerup:
					{
						ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.SpeedPowerup], true, &channel));
					}
					break;
				default:
					break;
				}

				// Set the sounds position and start the sound playing
				channel->setChannelGroup(m_channelGroupEffects);
				channel->set3DAttributes(&pos, 0);
				channel->setPaused(false);
			}
			break;
		case Events::EventType::Reset:
			{
				Events::ResetEvent *resetEvent = (Events::ResetEvent *)event;

				auto kart_local = m_karts[resetEvent->kart_id];

				if (kart_local != NULL)
				{
					auto kart_entity = GETENTITY(resetEvent->kart_id, CarEntity);

					FMOD_VECTOR pos;
					pos.x = kart_entity->Pos.x;
					pos.y = kart_entity->Pos.y;
					pos.z = kart_entity->Pos.z;

					ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.KartExplode],
						true, &(kart_local->soundsChannel)));
					kart_local->soundsChannel->set3DAttributes(&pos, 0);
					kart_local->soundsChannel->setChannelGroup(m_channelGroupEffects);
					kart_local->soundsChannel->setPaused(false);
				}
			}
			break;
		}
	}
	m_pMailbox->emptyMail();

	UpdateListenerPos();
	ERRCHECK(m_system->update());

	//OutputMemUsage();
}


