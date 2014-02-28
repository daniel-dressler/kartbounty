#ifndef NODE_H
#define NODE_H

#include <set>
#include <cmath>

class Node
{
	private:
		float pos_x;
		float pos_y;

	public:
		int id;
		std::set<int> neighbours;

	Node(int i, float x, float y) 
	{ 
		id = i;
		pos_x = x;
		pos_y = y;
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
