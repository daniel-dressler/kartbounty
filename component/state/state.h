#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

typedef struct StateData
{
	Real	fTime;
	Real	fElapse;

	struct
	{
		Vector3		vPos;
		Quaternion	qOrient;

	} Karts[4];

	btTriangleMesh	bttmArena;

} StateData;

extern StateData& GetState();

#endif
