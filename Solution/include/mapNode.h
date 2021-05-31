/*! \file mapNode.h
* \brief Header file for the nodes used for pathfinding (The MapNode class).
*
* Contains information about the map nodes.
*/

#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
// What is contained in the section
enum Object { UNKNOWN, OWNBASE, PLAYERBASE, PLAYERTANK, PLAYERSHELL };

/*! \class MapNode
* \brief Individual map nodes for the AI tank.
*
* Used for pathfinding for the AI tank.
*/
class MapNode : public sf::Drawable
{
private:
	sf::RectangleShape debugRect; //!< To show in debug mode

	Object contains; //!< What is in the section of the map
	sf::FloatRect border; //!< The border for the node

	// Values for A* searching
	float fTotalCost; //!< Total score for the node
	float fHeuristicScore; //!< Distance from the current to the goal node

	bool bPath; //!< If it is part of a path
public:
	MapNode() {} //!< Default constructor for MapNode.
	MapNode(sf::Vector2f newPosition, sf::Vector2f newSize, Object type); //!< Constructor for MapNode.

	bool bSeen; //!< If it can currently be seen

	void updateType(Object newType); //!< To change what is contained in the section
	Object getObjectType() const; //!< Returns the Object type
	sf::FloatRect getBorder() const; //!< Returnd the global border

	// More values for A* searching
	float fGeogScore; //!< Number of nodes to be traversed from this node to the goal
	int iIndex; //!< Index value of this node
	int iParentIndex; //!< Index value of the parent node
	void score(float parentG, int x, int y, int goalX, int goalY); //!< Function to score a node when A* searching
	bool operator<(const MapNode& other); //!< Operator for sorting the nodes in a list
	void setIfPath(bool is); //!< Set if it is a path
	bool isPath() { return bPath; } //!< Return if it is a path
	void resetColour(); //!< Reset the colour of the node

	//! Draws map nodes.
	/*!
	* \param target The target being rendered to.
	* \states Render states.
	*/
	void draw(sf::RenderTarget &target, sf::RenderStates states) const;
};