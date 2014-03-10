#ifndef _SPHERE_H
#define _SPHERE_H

#include "math.h"
#include "../../../Standard.h"

class Sphere
{
	Vector3 center;
	double radius;
	
	public:
	
	Sphere ()
	{
		center = Vector3(0,0,0);
		radius = 1.0;
	};
	
	Sphere (Vector3 centerValue, double radiusValue) 
	{
		center = centerValue;
		radius = radiusValue;
	};
	
	// method functions
	Vector3 getSphereCenter () { return center; };
	double getSphereRadius () { return radius; };
};

#endif
