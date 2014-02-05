#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"

typedef struct StateData
{
	Real	fTime;
	Real	fElapse;

	struct
	{
		Vector3		vPos;
		Quaternion	qOrient;

	} Karts[4];

	struct 
	{
		Vector3		focus;
		Vector3		cPos;
	} Camera;

} StateData;

extern StateData& GetState();

#endif
