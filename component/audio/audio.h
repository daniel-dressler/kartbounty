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
	std::vector<FMOD::Channel *>	m_SoundsChannelList;

	FMOD::ChannelGroup *m_channelMusic;
	FMOD::ChannelGroup *m_channelEffects;
	FMOD::ChannelGroup *m_channelEngineSound;

	FMOD::DSP *m_pitchShift;
	FMOD::DSP *m_lowfreqPitchShift;

	Real enginePitch;

	int LoadSound(char* file);
	int LoadMusic(char* file);

	void StartMusic();

	void SetupEngineSounds();
	void Setup3DEnvironment();
	void OutputMemUsage();

	void UpdateListenerPos();
	void UpdateKartsPos();

public:
	Audio();
	~Audio();

	struct Sounds
	{
		int PowerUp;
		int LowFreqEngine;
		int MachineGun;
	} Sounds;
	
	int SetupHardware();
	void PlayMusic(int id);
	void PlaySoundEffect(int id, Vector3 pos);

	void Update(Real seconds);
};