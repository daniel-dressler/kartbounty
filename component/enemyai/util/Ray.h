#ifndef _RAY_H
#define _RAY_H

#include "../../../Standard.h"

class Ray 
{
	Vector3 origin, direction;
	
	public:
		Ray ();
	
		Ray (Vector3 orig, Vector3 dir);
	
		// method functions
		Vector3 getRayOrigin () { return origin; }
		Vector3 getRayDirection () { return direction; }
	
};

Ray::Ray () 
{
	origin = Vector3(0,0,0);
	direction = Vector3(1,0,0);
}

Ray::Ray (Vector3 o, Vector3 d) 
{
	origin = o;
	direction = d;
}

#endif
