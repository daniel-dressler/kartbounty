#pragma once

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#ifdef _WIN32			// Order of this matters on windows

#define _SCL_SECURE_NO_WARNINGS
#define _STL_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define WIN_32_LEAN_AND_MEAN
#include <Windows.h>

#include <crtdbg.h>
#define DEBUGOUT( str, ... ) { char out[4096]; sprintf_s( out, 4096, str, ##__VA_ARGS__ ); OutputDebugString( out ); }

#else // Linux

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEBUGOUT( str, ... ) { fprintf( stderr, str, ##__VA_ARGS__ ); }

long _filelength(int fd)
{
	struct stat sb;
	fstat(fd, &sb);
	return sb.st_size;
}

#endif

// Common opengl includes are the same on windows and linux, only mac differs
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

// SDL recomends a linking directive which makes this equivilent on all platforms
#include <SDL.h>
#include <SDL_syswm.h>

#include "SELib/SELinearMath.h"
