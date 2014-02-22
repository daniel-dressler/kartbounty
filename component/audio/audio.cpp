#include "audio.h"

Audio::Audio() {
	// Initialize SDL Mixer functionality
	Mix_Init( MIX_INIT_FLAC | MIX_INIT_MP3 );

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
	fprintf(stderr, "Unable to initialize audio: %s\n", Mix_GetError());
	exit(1);
	}

	int pFrequency;
	Uint16 pFormat;
	int pAudioChannels;

	Mix_QuerySpec(&pFrequency, &pFormat, &pAudioChannels);
	DEBUGOUT("Audio opened with frequency: %d, channels: %d, format: %d\n", pFrequency, pAudioChannels, pFormat);

	//Load music files
	bgMusic = Mix_LoadMUS("assets/audio/music1.mp3");

	//Load Sound Samples
	//sfx1 = Mix_LoadWAV("assets/audio/DescendingCrash.wav");

	Mix_PlayMusic(bgMusic, -1);
	//Mix_PlayChannel(-1, sfx1, 5);
}

Audio::~Audio() {
	Mix_HaltMusic();
	Mix_FreeMusic(bgMusic);
	Mix_CloseAudio();
	while(Mix_Init(0))
	{
		Mix_Quit();
	}
}
