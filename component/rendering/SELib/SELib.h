#ifndef __SE_LIB__
#define __SE_LIB__

#define _ERR -1
#define _OK 0
#define _WARN 1
#define Failed( x ) ( x < 0 )
#define SafeDelete(x)			{if(x){delete x;x=0;}}

#include "SEStdDataTypes.h"
#include "SEStdMemory.h"
#include "SEStdMath.h"

#include "SETimer.h"

#include "SELinearMath.h"
#include "SECompData.h"
#include "SEMesh.h"

#endif
