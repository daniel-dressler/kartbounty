#ifndef __STANDARD__
#define __STANDARD__

#ifdef _WIN32			// Order of this matters on windows

#define _SCL_SECURE_NO_WARNINGS
#define _STL_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define WIN_32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include <crtdbg.h>
#define DEBUGOUT( str, ... ) { char out[4096]; sprintf_s( out, 4096, str, ##__VA_ARGS__ ); OutputDebugString( out ); }

#else // Linux

// Add linux OpenGL and SDL includes here

#define DEBUGOUT( str, ... ) { printf( str, ##__VA_ARGS__ ); }

#endif

#include <stdio.h>

#include "SELib/SELinearMath.h"

#endif
