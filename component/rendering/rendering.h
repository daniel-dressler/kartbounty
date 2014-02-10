#ifndef __RENDERING__
#define __RENDERING__

#include "../../Standard.h"
#include "SELib/SELib.h"
#include "glhelpers.h"
#include "ShaderStructs.h"

int InitRendering();
int UpdateRendering( float fElapseSec );
int Render();
int ShutdownRendering();

#endif
