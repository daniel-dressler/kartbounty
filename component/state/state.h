#ifndef __STATE_DATA__
#define __STATE_DATA__

#include "../rendering/SELib/SELib.h"
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

// HACK!
// StateData is temporary. Everything in here
// shall be replaced by events or the entity
// inventory when such exists.
// HACK!

#define MAX_POWERUPS	2
#define NUM_KARTS	3
#define PLAYER_KART 0

typedef struct StateData
{
	struct
	{
		Vector3		vPos;
		Vector3		vOldPos;
		Quaternion	qOrient;
		Vector3		vUp;
		btVector3	forDirection;
		Vector4		vColor;
		float		vSpeed;

		// This is where the AI is planning to go
		bool		isPlayer;
		Vector3		target_to_move;
		int			TimeStartedTarget;

		// physics info
		float	gVehicleSteering;
		float	gEngineForce;
		float	gBrakingForce ;

		float	wheelFriction ;
		float	suspensionStiffness ;
		float	suspensionDamping ;
		float	suspensionCompression ;
		float	rollInfluence ; // Keep low to prevent car flipping
		float	suspensionTravelcm ;

	} Karts[NUM_KARTS];

	struct
	{
		Int32		bEnabled;
		Int32		nType;
		Vector3		vPos;
		int			pointId;
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
