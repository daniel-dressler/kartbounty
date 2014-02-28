#ifndef POSITION_AI_H
#define POSITION_AI_H

#include "../../../Standard.h"

class Position
{
	public:	
		btScalar posX;
		btScalar posY;
		Position(btScalar x, btScalar y)
		{
			posX = x;
			posY = y;
		};
};
#endif