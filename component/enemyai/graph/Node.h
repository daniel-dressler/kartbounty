#ifndef NODE_H
#define NODE_H

#include <set>
#include <cmath>

#include "../../../Standard.h"

class Node
{
	

	private:
		int id_num;
		float pos_x;
		float pos_y;
		float height;

	public:
		int id;
		std::set<int> neighbours;

	Node(float x, float y) 
	{ 
		id = id_num++;

		pos_x = x;
		pos_y = y;
		height = 0;
	};

	Node(float x, float y, float z) 
	{ 
		id = id_num++;

		pos_x = x;
		pos_y = z;
		height = y;
	};

	btVector3 getBtVector()
	{
		return btVector3(pos_x,height,pos_y);
	};

	Vector3 getVector()
	{
		return Vector3(pos_x,height,pos_y);
	};

	// Get the actual distance between two points on the graph. Used as weights for the graph.
	float getDist(Node b)
	{
		return sqrtf( abs( pow( ( getPosX()-b.getPosX() ) , 2 ) + pow(( getPosY()-b.getPosY() ),2) ) );
	};

	public: std::set<int> getNeighbours()
	{
		return neighbours;
	}

	// Add the id of a node to neighbours.
	void addNeighbour(Node n)
	{
		neighbours.insert(n.getId());
	};

	public: bool isNeightbour(Node n)
	{
		return (neighbours.find(n.getId()) != neighbours.end());
	};

	// Setters
	void setId(int i) {id = i;};
	void setPosX(float x) {pos_x = x;};
	void setPosY(float y) {pos_y = y;};

public: 
	// Getters
	int getId() {return id;};
	float getPosX() {return pos_x;};
	float getPosY() {return pos_y;};

};

#endif
