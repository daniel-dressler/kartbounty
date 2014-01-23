#ifndef __RENDERING__
#define __RENDERING__

#include "../../Standard.h"
#include "SELib/SELib.h"
#include "glhelpers.h"
#include "ShaderStructs.h"

int InitRendering( SDL_Window* win );
int UpdateRendering( float fElapseSec );
int Render();
int ShutdownRendering();

#endif
