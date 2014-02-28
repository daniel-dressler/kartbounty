#pragma once

#include "../../Standard.h"
#include "../events/events.h"
#include <fmod.hpp>

class Audio {
private:
	FMOD::System *m_system;
	Events::Mailbox *m_pMailbox;

	std::vector<FMOD::Sound *> m_SoundList;
	std::vector<FMOD::Sound *> m_MusicList;

	FMOD::ChannelGroup *channelMusic;
	FMOD::ChannelGroup *channelEffects;

	int LoadSound(char* file);
	int LoadMusic(char* file);
	void OutputMemUsage();

	void UpdateListenerPos();

public:
	Audio();
	~Audio();

	int Music1;
	
	int SetupHardware();
	void PlayMusic(int id);
	void PlaySoundEffect(int id, Vector3 pos);

	void Update();
};