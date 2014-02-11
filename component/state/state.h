#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

// HACK!
// StateData is temporary. Everything in here
// shall be replaced by events or the entity
// inventory when such exists.
// HACK!
typedef struct StateData
{
	struct
	{
		Vector3		vPos;
		Quaternion	qOrient;

	} Karts[4];

	struct 
	{
		Real		fFOV;
		Vector3		vFocus;
		Vector3		vPos;
		btVector3	oldVec;
	} Camera;

	btTriangleMesh	*bttmArena;

	bool key_map[256];

	StateData();
	~StateData();
} StateData;

extern StateData& GetState();
extern StateData *GetMutState();

#endif
