#pragma once

#include "../../Standard.h"
#include "../events/events.h"
#include <fmod.hpp>

class Audio {
private:
	FMOD::System *m_system;
	Events::Mailbox *m_pMailbox;

	std::vector<FMOD::Sound *>		m_SoundList;
	std::vector<FMOD::Sound *>		m_MusicList;

	std::vector<FMOD::Channel *>	m_EngineChannelList;
	std::vector<FMOD::Sound *>		m_EngineSoundList;
	std::vector<FMOD::DSP *>		m_KartEngineDSPList;

	FMOD::ChannelGroup *m_channelMusic;
	FMOD::ChannelGroup *m_channelEffects;
	FMOD::ChannelGroup *m_channelEngineSound;

	FMOD::DSP *m_pitchShift;

	int LoadSound(char* file);
	int LoadMusic(char* file);

	void SetupEngineSounds();
	void Setup3DEnvironment();
	void OutputMemUsage();

	void UpdateListenerPos();

public:
	Audio();
	~Audio();

	struct Sounds
	{
		int PowerUp;
	} Sounds;
	
	int SetupHardware();
	void PlayMusic(int id);
	void PlaySoundEffect(int id, Vector3 pos);

	void Update();
};