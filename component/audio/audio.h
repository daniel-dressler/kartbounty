#pragma once

#include <fmod.hpp>

#include "../../Standard.h"
#include "../events/events.h"
#include "../entities/entities.h"

class Audio {
private:
	FMOD::System *m_system;
	Events::Mailbox *m_pMailbox;

	std::vector<FMOD::Sound *>		m_SoundList;
	std::vector<FMOD::Sound *>		m_MusicList;

	std::vector<FMOD::Sound *>		m_EngineSoundList;

	FMOD::ChannelGroup *m_channelGroupMusic;
	FMOD::ChannelGroup *m_channelGroupEffects;
	FMOD::ChannelGroup *m_channelGroupEngineSound;

	FMOD::DSP *m_pitchShift;

	Real enginePitch[NUM_KARTS];

	struct kart_audio {
		FMOD::Channel *idleNoiseChannel;
		FMOD::Channel *engineChannel;
		FMOD::Channel *soundsChannel;
		FMOD::DSP     *engineDSP;
		Real enginePitch;
		entity_id kart_id;
		kart_audio() {
			memset(this, 0, sizeof(*this));
		}
	};
	std::map<entity_id, struct kart_audio *> m_karts;

	// We have to play sound from a single
	// kart's perspective. This player
	// shall be that perspective.
	entity_id primary_player;

	int LoadSound(char* file);
	int LoadMusic(char* file);

	bool playMusic;

	float musicVol;
	float sfxVol;

	void StartMusic();
	void ToggleMusic();

	void SetupEngineSounds(struct kart_audio *);
	void Setup3DEnvironment();
	void OutputMemUsage();

	void UpdateListenerPos();
	void UpdateKartsPos(struct kart_audio *);

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
	void setup();
};
