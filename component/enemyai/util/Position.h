#ifndef POSITION_AI_H
#define POSITION_AI_H

#include "../../../Standard.h"

class Position
{
	public:	
		btScalar posX;
		btScalar posY;
		btScalar height;

		Position(btScalar x, btScalar y)
		{
			posX = x;
			posY = y;
			height = 0;
		};

		Position(btScalar x, btScalar y, btScalar h)
		{
			posX = x;
			posY = y;
			height = h;
		};

		btVector3 getBtVector()
		{
			return btVector3(posX,height,posY);
		};

		Vector3 getVector()
		{
			return Vector3(posX,height,posY);
		};
};
#endif