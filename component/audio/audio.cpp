#include "audio.h"
#include <fmod.h>
#include <fmod_errors.h>

#define ERRCHECK(res)	if (res != FMOD_OK){\
	DEBUGOUT("FMOD error! (%d) %s\n", res, FMOD_ErrorString(res));\
    exit(-1);}

Audio::Audio() {
	
	m_pMailbox = new Events::Mailbox();	
	m_pMailbox->request( Events::EventType::Input );
	
	SetupHardware();

	// Setup music and sound effects channels
	ERRCHECK(m_system->createChannelGroup(NULL, &channelEffects));
	ERRCHECK(m_system->createChannelGroup(NULL, &channelMusic));

	// Load all the sound files
	LoadMusic("assets/audio/music1.mp3");
	LoadSound("assets/audio/DescendingCrash.wav");

	FMOD::Channel *channel;
	m_system->playSound(FMOD_CHANNEL_FREE, m_MusicList[0], 0, &channel);
}

Audio::~Audio() {

	m_system->release();

	if(m_MusicList.size() > 0)
	{
		for(int i = m_MusicList.size()-1; i <= 0; i--)
		{
			FMOD::Sound music = *m_MusicList[i];
			music.release();
		}
	}

	if(m_SoundList.size() > 0)
	{
		for(int i = m_SoundList.size()-1; i <= 0; i--)
		{
			FMOD::Sound sound = *m_MusicList[i];
			sound.release();
		}
	}

	if(m_pMailbox)
		delete m_pMailbox;
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
	FMOD_RESULT result;
	FMOD::Sound *newSound;

	result = m_system->createSound(file, FMOD_3D, 0, &newSound);
	ERRCHECK(result);

	m_SoundList.push_back(newSound);

	return m_SoundList.size() - 1;
}

void Audio::UpdateListenerPos(){
	FMOD_VECTOR *position;
	FMOD_VECTOR *velocity;
	FMOD_VECTOR *forward;
	FMOD_VECTOR *up;

	//GetState().Karts

	//m_system->set3DListenerAttributes(0, 
	FMOD_CHANNELGROUP *group;
}

void Audio::Update(){

	// Check for input events
	if( m_pMailbox )
	{
		const std::vector<Events::Event*> aryEvents = m_pMailbox->checkMail();
		for( unsigned int i = 0; i < aryEvents.size(); i++ )
		{
			if(aryEvents[i]->type == Events::EventType::Input)
			{
				Events::InputEvent *input = (Events::InputEvent *)aryEvents[i];
				if(input->aPressed)
				{
					// Play a sound effect

				}
			}
		}
		m_pMailbox->emptyMail();
	}
	m_system->update();
	OutputMemUsage();
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
}


