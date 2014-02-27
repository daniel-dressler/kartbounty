#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

// HACK!
// StateData is temporary. Everything in here
// shall be replaced by events or the entity
// inventory when such exists.
// HACK!

#define MAX_POWERUPS	12
#define NUM_KARTS	4
typedef struct StateData
{
	struct
	{
		Vector3		vPos;
		Vector3		vOldPos;
		Quaternion	qOrient;
		btVector3	forDirection;

	} Karts[NUM_KARTS];

	struct
	{
		Int32		bEnabled;
		Int32		nType;
		Vector3		vPos;
	} Powerups[MAX_POWERUPS];

	struct 
	{
		Real		fFOV;
		Vector3		vFocus;
		Vector3		vPos;
	} Camera;

	btTriangleMesh	*bttmArena;

	bool key_map[256];

	StateData();
	~StateData();
} StateData;

extern StateData& GetState();
extern StateData *GetMutState();

#endif
