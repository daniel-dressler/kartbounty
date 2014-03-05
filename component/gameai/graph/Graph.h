#ifndef GRAPH_H
#define GRAPH_H

#include "Node.h"
#include <map>
#include <vector>
#include <iostream>

#define MAX_VALUE 1000000;

class Graph
{
	std::map<int, Node> nodes;

	// Add a node to the graph
	public: void addNode(Node n)
	{
		nodes.insert( std::pair <int, Node> (n.getId(), n));
	};

	// Both direction neighbours.
	public: void addEdge(Node n, Node n2)
	{
		if (!n.isNeightbour(n2))
			nodes.at(n.getId()).addNeighbour(nodes.at(n2.getId()));

		if (!n2.isNeightbour(n))
			nodes.at(n2.getId()).addNeighbour(nodes.at(n.getId()));
	};

	public: int size()
	{
		return (nodes.size());
	};


	public: Node getNode(int i)
	{
		return nodes.at(i);
	};

	public: Node getRandomNode()
	{
		int i = (rand() % size());

		return nodes.at(i);
	};

	public: void connectAll()
	{
		for (auto n: nodes)
		{
			for (auto n2: nodes)
			{
				if (n.second.getId() != n2.second.getId())
					addEdge(n.second, n2.second);
			}
		}
	}

	// Does the distance map for each node using Dijkstra's Algorithm.
	public: std::map<int, int> findShortestPath(Node node)
	{
		std::map<int, double> dist_map; // id of the node I want to go to, it's current distance.

		// init for all to be max value.
		for (auto node: nodes)
		{
			dist_map[node.first] = MAX_VALUE;
		}

		std::map<int, int> parent_map; // id of the node I want to go to, id of parent.

		// init for all to be max value (no parent).
		for (auto node: nodes)
		{
			parent_map[node.first] = MAX_VALUE;
		}

		std::vector<int> not_completed; // id's of nodes not yet completed.

		// init, push all.
		for (auto node: nodes)
		{
			not_completed.push_back(node.first);
		}

		Node current = getNode(node.getId()); // Set the current to be the first.
		dist_map[current.id] = 0;
		parent_map[current.id] = 0;

		while(!not_completed.empty())
		{
			// Find all distances from current. 
			//		Update parant/dist map if better.
			//
			// Remove selected from not_complete.
			// Select the next min one.

			// Check ALL neighbour distances.
			std::set<int> neighbours = current.getNeighbours();

			for (auto node_id : neighbours)
			{
				Node temp = getNode(node_id);
				double dist = current.getDist(temp) + dist_map[current.getId()]; // The distance between the two points and the distance to current.
					
				// if the distance is smaller then before, update dist and parent maps.
				if (dist_map[node_id] > dist)
				{
					dist_map[node_id] = dist;
					parent_map[node_id] = current.getId();
				}
			}

			// Find the current element's position.
			int pos = find(not_completed.begin(), not_completed.end(), current.getId()) - not_completed.begin();	
			not_completed.erase (not_completed.begin() + pos); // erase that element

			// if this wasn't the last, find the next minimum value
			if (!not_completed.empty())
				current = find_next_current(not_completed, dist_map);
		}

		/* 
		//DEBUG print out of the maps.
		std::cout << "Distance map: ";
		std::cout.precision(7);
		for (int i=0; i<dist_map.size(); i++)
		{
			std::cout << dist_map[i] << "(" << i << ") ";
		}
		std::cout << std::endl;

		std::cout << "Parent map: ";
		for (int i=0; i<parent_map.size(); i++)
		{
			std::cout << parent_map[i] << "(" << i << ") ";
		}
		std::cout << std::endl;
		*/

		return parent_map;
	};

	Node find_next_current(std::vector<int> not_completed, std::map<int, double> dist_map)
	{
		int min_dist = MAX_VALUE;
		int min_node_id = -1;

		for (int node_id: not_completed)
		{
			if (dist_map[node_id] < min_dist)
			{
				min_node_id = node_id;
			}
		}

		return getNode(min_node_id);
	}
			
	std::vector<int> getPath(Node from_node, Node to_node)
	{
		std::map<int, int> parent_map = findShortestPath(from_node);
		std::vector<int> path_int;
		int current_id = to_node.getId(); // 2

		while (current_id != from_node.getId())
		{
			path_int.push_back(current_id);
			current_id = parent_map[current_id];
		}

		std::reverse(path_int.begin(),path_int.end());

		return path_int;
	}
};

#endif
