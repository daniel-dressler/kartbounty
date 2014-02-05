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

<<<<<<< HEAD
	struct 
	{
		Vector3		focus;
		Vector3		cPos;
	} Camera;
=======
	btTriangleMesh	bttmArena;
>>>>>>> 426eefe357c047a15b4486c054680238a0e8cdec

} StateData;

extern StateData& GetState();

#endif
