#include "Standard.h"

#ifdef _WIN32

#pragma comment( lib, "glew32.lib" )
#pragma comment( lib, "opengl32.lib" )

#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "SDL2main.lib" )

#else

long _filelength(int fd)
{
	struct stat sb;
	fstat(fd, &sb);
	return sb.st_size;
}

#endif

