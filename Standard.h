#pragma once

#define GAMENAME "KartBounty"

#ifdef _WIN32

#define _SCL_SECURE_NO_WARNINGS
#define _STL_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#endif

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <algorithm>

#ifdef _WIN32			// Order of this matters on windows


#define WIN_32_LEAN_AND_MEAN
#include <Windows.h>

#include <crtdbg.h>
#define DEBUGOUT( str, ... ) { char out[99999]; sprintf_s( out, 99999, str, ##__VA_ARGS__ ); OutputDebugString( out ); }
//#define DEBUGOUT( str, ... ) {}

#ifdef _DEBUG
#define INIT_PLATFORM() \
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF )
#else
#define INIT_PLATFORM()
#endif

#else // Linux

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEBUGOUT( str, ... ) { fprintf( stderr, str, ##__VA_ARGS__ ); }

long _filelength(int fd);

// Hack, SDL2.0.1 is not in repos
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)

//#define max(x,y) fmax(x,y)

#define INIT_PLATFORM()
// Stub for audio.h
class Audio {
	public:
	void update(double std) {
		return;
	}
	void setup() {
		return;
	}
};
#endif

// Common opengl includes are the same on windows and linux, only mac differs
#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>	// I think this is for x-windows
#endif

// SDL recomends a linking directive which makes this equivilent on all platforms
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_timer.h>

#include "component/rendering/SELib/SELib.h"

