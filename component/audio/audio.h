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
	std::vector<FMOD::Channel *>	m_SoundChannelList;
	std::vector<FMOD::Sound *>		m_MusicList;

	std::vector<FMOD::Sound *>		m_EngineSoundList;

	FMOD::ChannelGroup *m_channelGroupMusic;
	FMOD::ChannelGroup *m_channelGroupEffects;
	FMOD::ChannelGroup *m_channelGroupEngineSound;

	FMOD::DSP *m_pitchShift;
	FMOD::Channel *roundStartChannel;
	FMOD::Channel *musicPlaybackChannel;

	bool gamePaused;

	struct kart_audio {
		FMOD::Channel *idleNoiseChannel;
		FMOD::Channel *engineChannel;
		FMOD::Channel *collisionChannel;
		FMOD::Channel *bulletHitChannel;
		FMOD::Channel *soundsChannel;
		FMOD::Sound   *engineSound;
		FMOD::DSP     *engineDSP;
		float enginePitch;
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

	int LoadSound(char* file, FMOD_MODE mode);
	int LoadMusic(char* file);

	bool playMusic;
	int m_musicTrack;

	float musicVol;
	float sfxVol;

	void StartMusic();
	void ToggleMusic();

	void SetupEngineSounds(struct kart_audio *);
	void DestroyEngineSounds(struct kart_audio *);
	void Setup3DEnvironment();
	void OutputMemUsage();

	void UpdateListenerPos();
	void UpdateKartsPos(entity_id);

public:
	Audio();
	~Audio();

	struct Sounds
	{
		int EngineSound;
		int EngineIdleSound;
		int PowerUpPickUp;
		int HealthPowerup;
		int SpeedPowerup;
		int RocketPowerup;
		int GoldChestPowerup;
		int LowFreqEngine;
		int MachineGun;
		int WallCollision;
		int Skid;
		int KartExplode;
		int KartBulletHit;
		int RoundStart;
		int Boo;
		int Cheer;
		int PulsePowerUp;
	} Sounds;
	
	int SetupHardware();
	void PlayMusic(int id);
	void PlaySoundEffect(int id, Vector3 pos);
	void ChangeMusic();

	void update(Real seconds);
	void setup();
};
