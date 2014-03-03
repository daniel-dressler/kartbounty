#ifndef _SQUARE_H
#define _SQUARE_H

#include "math.h"
#include "../../../Standard.h"


class Square
{
	Vector3 center;
	Vector3 topLeft, topRight, bottomRight, bottomLeft;

public:
	Square (Vector3 centerValue, Vector3 tl, Vector3 br) 
	{
		Vector3 tr = Vector3(tl.z, tl.y, br.x);
		Vector3 bl = Vector3(tl.x, tl.y, br.z);

		center = centerValue;
		topLeft = tl; 
		topRight = tr;
		bottomRight = br; 
		bottomLeft = bl;
		
	};

	// method functions
	Vector3 getCenter () { return center; };
	Vector3 getTopLeft() { return topLeft; } ;
	Vector3 getTopRight() { return topRight; } ;
	Vector3 getBottomLeft() { return bottomLeft; } ;
	Vector3 getBottomRight() { return bottomRight; } ;

	struct Point
	{
		float x;
		float y;
	};

	// Given three colinear points p, q, r, the function checks if
	// point q lies on line segment 'pr'
	bool onSegment(Point p, Point q, Point r)
	{
		if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
			q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
		   return true;
 
		return false;
	}

	// To find orientation of ordered triplet (p, q, r).
	// The function returns following values
	// 0 --> p, q and r are colinear
	// 1 --> Clockwise
	// 2 --> Counterclockwise
	int orientation(Point p, Point q, Point r)
	{
		// See 10th slides from following link for derivation of the formula
		// http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf
		float val = (q.y - p.y) * (r.x - q.x) -
				  (q.x - p.x) * (r.y - q.y);
 
		if (val == 0) return 0;  // colinear
 
		return (val > 0)? 1: 2; // clock or counterclock wise
	}
 
	// The main function that returns true if line segment 'p1q1' and 'p2q2' intersect.
	// Source: http://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/
	bool doIntersect(Point p1, Point q1, Point p2, Point q2)
	{
		// Find the four orientations needed for general and
		// special cases
		int o1 = orientation(p1, q1, p2);
		int o2 = orientation(p1, q1, q2);
		int o3 = orientation(p2, q2, p1);
		int o4 = orientation(p2, q2, q1);
 
		// General case
		if (o1 != o2 && o3 != o4)
			return true;
 
		// Special Cases
		// p1, q1 and p2 are colinear and p2 lies on segment p1q1
		if (o1 == 0 && onSegment(p1, p2, q1)) return true;
 
		// p1, q1 and p2 are colinear and q2 lies on segment p1q1
		if (o2 == 0 && onSegment(p1, q2, q1)) return true;
 
		// p2, q2 and p1 are colinear and p1 lies on segment p2q2
		if (o3 == 0 && onSegment(p2, p1, q2)) return true;
 
		 // p2, q2 and q1 are colinear and q1 lies on segment p2q2
		if (o4 == 0 && onSegment(p2, q1, q2)) return true;
 
		return false; // Doesn't fall in any of the above cases
	}
 
	// Corenrs has to be fed first!
	int LineIntersectsSquare(Vector3 rayOrigin, Vector3 direction, float distanceOnVect)
	{
		int intersections = 0;

		Vector3 rayFinalPoint = rayOrigin + (direction*distanceOnVect);
	
		struct Point orgRay = {rayOrigin.x, rayOrigin.z};
		struct Point targRay = {rayFinalPoint.x, rayFinalPoint.z};

		//DEBUGOUT("ORIGIN : %f, %f\n", orgRay.x, orgRay.y);
		//DEBUGOUT("TARGET : %f, %f\n", targRay.x, targRay.y);

		struct Point top_left = {topLeft.x, topLeft.z};
		struct Point bottom_left = {bottomLeft.x, bottomLeft.z};
		struct Point top_right = {topRight.x, topRight.z};
		struct Point bottom_right = {bottomRight.x, bottomRight.z};

		//DEBUGOUT("TOP LEFT : %f, %f\n", top_left.x, top_left.y);
		//DEBUGOUT("TOP RIGHT : %f, %f\n", top_right.x, top_right.y);
		//DEBUGOUT("BOT RIGHT : %f, %f\n", bottom_right.x, bottom_right.y);
		//DEBUGOUT("BOT LEFT : %f, %f\n", bottom_left.x, bottom_left.y);

		// Left side, topLeft and botLeft
		int isInter = doIntersect(orgRay, targRay, top_left, bottom_left);
		if (isInter == 1 || isInter == 2)
		{
			intersections++;
		}

		// Right side, topRight and botRight
		isInter = doIntersect(orgRay, targRay, top_right, bottom_right);
		if (isInter == 1 || isInter == 2)
		{
			intersections++;
		}

		// Top side, topRight and topLeft
		isInter = doIntersect(orgRay, targRay, top_right, top_left);
		if (isInter == 1 || isInter == 2)
		{
			intersections++;
		}

		// Bot side, botRight and botLeft
		isInter = doIntersect(orgRay, targRay, bottom_right, bottom_left);
		if (isInter == 1 || isInter == 2)
		{
			intersections++;
		}

		return intersections;
	};
};

#endif
