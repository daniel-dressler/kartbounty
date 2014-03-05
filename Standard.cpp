#include "Standard.h"

#ifdef _WIN32

#pragma comment( lib, "glew32.lib" )
#pragma comment( lib, "opengl32.lib" )

#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2main.lib" )
#pragma comment( lib, "fmodexL_vc.lib")

#ifndef _DEBUG
#pragma comment( lib, "BulletPhysics.lib" )
#else
#pragma comment( lib, "BulletPhysicsD.lib" )
#endif

#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "version.lib" )
#pragma comment( lib, "imm32.lib" )

#else

long _filelength(int fd)
{
	struct stat sb;
	fstat(fd, &sb);
	return sb.st_size;
}

#endif

