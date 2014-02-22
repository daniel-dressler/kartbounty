#pragma once

#include "../../Standard.h"
#include "../events/events.h"

class Audio {
private:
	Mix_Music *bgMusic;
	Mix_Chunk *sfx1;
	
	enum SoundChunks
	{
		BOOM
	};
	std::map<SoundChunks, Mix_Chunk> sfxList;

	// Audio Mixer Settings
	const static int audio_rate = 44100;
	const static Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
	const static int audio_channels = 2;
	const static int audio_buffers = 4096;		// Change this if sounds get too delayed

public:
	Audio();
	~Audio();

	void PlaySound(SoundChunks sound, Vector3 position);

	void Update();
};