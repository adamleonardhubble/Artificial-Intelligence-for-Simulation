/*! \file map.h
* \brief Header file for the map used for pathfinding (The Map class).
*
* Contains information about the map, and methods related to the pathfinding around said map.
*/

#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <list>
#include <stack>


#include "mapNode.h"
#include "position.h"

/*! \class Map
* \brief Map for the AI tank.
*
* Used for pathfinding for the AI tank.
*/
class Map : public sf::Drawable
{
private:
	static const int s_kiWidth = 19; //!< Number of columns of nodes.
	static const int s_kiHeight = 13; //!< Number of rows of nodes.
	static const int s_kiNodes = s_kiWidth * s_kiHeight; //!< Number of nodes.

	const float kiBackgroundWidth = 780.f; //!< Width of background.
	const float kiBackgroundHeight = 560.f; //!< Height of background.

	std::vector<std::vector<bool>> vbAdjacencyMatrix; //!< For if a node is next to another.
	int iIndexes[s_kiWidth][s_kiHeight]; //!< The number for each node using x and y values.

	//int iTankNotVisibleFrameCount = 0; 
	//const int kiTankNotVisibleFrameMin = 20;

	MapNode node[s_kiWidth][s_kiHeight]; //!< 2d array of nodes for the map.
	std::list<int> currentPath; //!< Current path being followed.
public:
	Map(); //!< Default constructor for Map.

	void mark(sf::FloatRect objectBounds, Object type); //!< To mark a found object on the map.
	void update(int i, int j, bool canSee, Position pos, sf::Vector2i goal); //!< To clear nodes.
	void makeNewPath(float x, float y, sf::Vector2i &goalNode); //!< Make a new path to follow.
	sf::Vector2f followPath(Position pos); //!< Called when following the path.
	sf::FloatRect getNodeBox(int i, int j); //!< To get the floatrect of the box.
	Object getNodeObject(int i, int j); //!< To get the object in the node.

	int getWidth() { return s_kiWidth; } //!< Return the width of the map.
	int getHeight() { return s_kiHeight; } //!< Return the height of the map.

	bool traversable(Object type); //!< Check if the node is traversable, returns true if it is.
	int index(int x, int y); //!< To return the number of the node.
	void inverseIndex(int index, int& x, int& y); //!< Get the x and y values of the node based on the index.
	void setMapTraversable(); //!< Call setAreaTraversable for whole map.
	void setAreaTraversable(int x1, int y1, int x2, int y2); //!< Set an area of nodes to if they are traversable (works faster than checking whole map every time).
	std::list<int> aStarSearch(int currentX, int currentY, int goalX, int goalY); //!< Generate a path using the A* search method.
	std::list<int> dfsSearch(int currentX, int currentY, int goalX, int goalY); //!< Generate a path using the DFS method.
	std::list<int> bfsSearch(int currentX, int currentY, int goalX, int goalY); //!< Generate a path using the BFS method.
	std::list<int> buildPath(std::list<MapNode>& searched); //!< Rebuilds the path from a list of nodes after searching.

	//! Draws map.
	/*!
	* \param target The target being rendered to.
	* \states Render states.
	*/
	void draw(sf::RenderTarget &target, sf::RenderStates states) const;
};