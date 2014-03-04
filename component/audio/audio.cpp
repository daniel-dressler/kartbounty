#include "audio.h"
#include <fmod.h>
#include <fmod_errors.h>

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
	m_pMailbox->request( Events::EventType::PowerupPickup );

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
	SetupEngineSounds();
}

Audio::~Audio() {

	m_system->release();

	if(m_pMailbox)
		delete m_pMailbox;
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

void Audio::SetupEngineSounds(){

	for(int i = 0; i < NUM_KARTS; i++)
	{
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

		FMOD_VECTOR *pos = new FMOD_VECTOR();
		pos->x = GetState().Karts[i].vPos.x;
		pos->y = GetState().Karts[i].vPos.y;
		pos->z = GetState().Karts[i].vPos.z;

		ERRCHECK(engineNoisechannel->set3DAttributes(pos, 0));
		ERRCHECK(idelNoiseChannel->set3DAttributes(pos, 0));
		ERRCHECK(engineNoisechannel->setMode(FMOD_LOOP_NORMAL));
		ERRCHECK(idelNoiseChannel->setMode(FMOD_LOOP_NORMAL));

		idelNoiseChannel->setVolume(LOW_ENGINE_NOISE_VOLUME);

		enginePitch[i] = 0;

		m_EngineSoundList.push_back(engineSound);
		m_EngineChannelList.push_back(engineNoisechannel);
		m_IdleNoiseChannelList.push_back(idelNoiseChannel);
		m_KartEngineDSPList.push_back(dsp);

		ERRCHECK(engineNoisechannel->setChannelGroup(m_channelGroupEngineSound));
		ERRCHECK(idelNoiseChannel->setChannelGroup(m_channelGroupEngineSound));

		ERRCHECK(engineNoisechannel->setPaused(false));
		ERRCHECK(idelNoiseChannel->setPaused(false));
	}
}

void Audio::Setup3DEnvironment() {
	ERRCHECK(m_system->set3DSettings(DOPPLER_SCALE, DISTANCE_FACTOR, ROLL_OFF_SCALE));
}

void Audio::OutputMemUsage(){
	float stream;
	float update;
	float total;

	m_system->getCPUUsage(0, &stream, 0, &update, &total);

	DEBUGOUT("Audio CPU usage:  Stream: %lf | Update: %lf | Total: %lf\n", stream, update, total);
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
	float speed = GetState().Karts[0].vSpeed;

	position.x = GetState().Karts[0].vPos.x;
	position.y = GetState().Karts[0].vPos.y;
	position.z = GetState().Karts[0].vPos.z;

	// Get the Kart's forward vector and remove the y component and then normalize to get unit vector length
	btVector3 temp = GetState().Karts[0].forDirection;
	temp.setY(0);
	temp.normalize();

	velocity.x = GetState().Karts[0].forDirection.x() * speed;
	velocity.y = GetState().Karts[0].forDirection.y() * speed;
	velocity.z = GetState().Karts[0].forDirection.z() * speed;

	forward.x = temp.getX();
	forward.y = temp.getY();
	forward.z = temp.getZ();

	up.x = 0;
	up.y = 1;
	up.z = 0;

	ERRCHECK(m_system->set3DListenerAttributes(0, &position, &velocity, &forward, &up));
}

void Audio::UpdateKartsPos(){
	for(int i = 0; i < NUM_KARTS; i++)
	{
		FMOD_VECTOR *pos = new FMOD_VECTOR();
		pos->x = GetState().Karts[i].vPos.x;
		pos->y = GetState().Karts[i].vPos.y;
		pos->z = GetState().Karts[i].vPos.z;
		ERRCHECK(m_EngineChannelList[i]->set3DAttributes(pos, 0));
		ERRCHECK(m_IdleNoiseChannelList[i]->set3DAttributes(pos, 0));
	}
}

void Audio::Update(Real seconds){

	// Check for keyboard events
	if (GetState().key_map['p']){
		ToggleMusic();
	}
	if (GetState().key_map['-']){
		sfxVol -= Clamp(sfxVol - 0.1f);
		ERRCHECK(m_channelGroupEffects->setVolume(sfxVol));		
	}

	// Check for input events
	if( m_pMailbox )
	{
		const std::vector<Events::Event*> aryEvents = m_pMailbox->checkMail();
		for( unsigned int i = 0; i < aryEvents.size(); i++ )
		{
			if(aryEvents[i]->type == Events::EventType::Input)
			{
				// Handle all one-off sound effects
				Events::InputEvent *input = (Events::InputEvent *)aryEvents[i];
				if(input->aPressed)  // Weapon fired
				{
					FMOD_VECTOR pos;
					int KartID = input->kart_index;

					pos.x = GetState().Karts[KartID].vPos.x;
					pos.y = GetState().Karts[KartID].vPos.y;
					pos.z = GetState().Karts[KartID].vPos.z;

					bool isPlaying = false;
					m_SoundsChannelList[KartID]->isPlaying(&isPlaying);

					if(!isPlaying)
					{
						ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[Sounds.MachineGun], true, &m_SoundsChannelList[KartID]));
						m_SoundsChannelList[KartID]->set3DAttributes(&pos, 0);
						m_SoundsChannelList[KartID]->setChannelGroup(m_channelGroupEffects);
						m_SoundsChannelList[KartID]->setPaused(false);
					}
				}
				if(input->bPressed)
				{
					// Play a sound effect
					int x = 0;
				}

				Real lerpAmt = seconds * 2.0f;
				Real newPitch = Lerp(enginePitch[input->kart_index], input->rightTrigger * MAX_PITCH, lerpAmt);

				enginePitch[input->kart_index] = newPitch;

				// Update Kart Engine Sounds
				ERRCHECK(m_KartEngineDSPList[input->kart_index]->setParameter(FMOD_DSP_PITCHSHIFT_PITCH, newPitch));

				m_EngineChannelList[input->kart_index]->setVolume(newPitch / 2);
				ERRCHECK(m_lowfreqPitchShift->setParameter(FMOD_DSP_PITCHSHIFT_PITCH, GetState().Karts[0].vSpeed / 25 * MAX_PITCH));

				//lowfreqChannel->setVolume(1 - (newPitch / 2));

			}
			else if(aryEvents[i]->type = (Events::EventType::PowerupPickup))
			{
				Events::PowerupPickupEvent *event = (Events::PowerupPickupEvent *)aryEvents[i];
				FMOD_VECTOR pos;
				int KartID = *event->picker_kart_index;

				pos.x = GetState().Karts[0].vPos.x;
				pos.y = GetState().Karts[0].vPos.y;
				pos.z = GetState().Karts[0].vPos.z;

				FMOD::Channel *channel;

				int PowerUp = Sounds.PowerUp;

				ERRCHECK(m_system->playSound(FMOD_CHANNEL_FREE, m_SoundList[PowerUp], true, &channel));
				channel->setChannelGroup(m_channelGroupEffects);
				channel->set3DAttributes(&pos, 0);
				channel->setPaused(false);
			}
		}
		m_pMailbox->emptyMail();
	}

	UpdateListenerPos();

	ERRCHECK(m_system->update());

	//OutputMemUsage();
}


