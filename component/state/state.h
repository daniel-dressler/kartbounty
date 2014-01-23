#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"

typedef struct StateData
{
	struct
	{
		Vector3		vPos;
		Quaternion	qOrient;

	} Karts[4];
} StateData;

extern StateData& GetState();

#endif
