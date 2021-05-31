/*! \file mapNode.cpp
* \brief Source file for the MapNode class.
*
* Contains the definitions for the MapNode class' constructor and methods.
*/

#include "mapNode.h"

MapNode::MapNode(sf::Vector2f newPosition, sf::Vector2f newSize, Object type)
{
	// Sets everything for the debug rectangle
	debugRect.setSize(newSize);
	debugRect.setOrigin(sf::Vector2f(newSize.x / 2.f, newSize.y / 2.f));
	debugRect.setPosition(newPosition);
	debugRect.setFillColor(sf::Color(0, 0, 0, 0));
	debugRect.setOutlineThickness(1.f);
	debugRect.setOutlineColor(sf::Color(0, 0, 0, 135));

	contains = type; // Sets the initial what is contained in the section
	border = debugRect.getGlobalBounds(); // Set the border of the node to the rectangles global boundary (Relative to the world coordinates)

	bPath = false;
	resetColour();
}

void MapNode::updateType(Object newType)
{
	contains = newType; // Sets what is in the section when the tank sees it

	resetColour(); // Set the nodes colour
}

Object MapNode::getObjectType() const
{
	return contains; // Return what it contains
}

sf::FloatRect MapNode::getBorder() const
{
	return border; // Return it's border
}

void MapNode::score(float parentG, int x, int y, int goalX, int goalY)
{
	fGeogScore = parentG + 1.f; // Parents geographical score + 1
	fHeuristicScore = pow(goalX - x, 2) + pow(goalY - y, 2); // The distance from the current node to tht goal
	fTotalCost = fGeogScore + fHeuristicScore; // Add them together for total score
}

bool MapNode::operator<(const MapNode& other)
{
	// Return true if the other total cost is more than this ones total cost
	return fTotalCost < other.fTotalCost;
}

void MapNode::setIfPath(bool is)
{
	bPath = is; // Set the stored bool if it is a path
	if (bPath) // If it is
		debugRect.setFillColor(sf::Color(255, 0, 0, 100)); // Set the debug rect to the path colour
}

void MapNode::resetColour()
{
	if (contains == Object::OWNBASE) // If it contains it's own base
	{
		debugRect.setFillColor(sf::Color(135, 0, 0, 100)); // Set the colour of the debug rectangle to red
	}
	if (contains == Object::PLAYERBASE || contains == Object::PLAYERTANK) // If it contains a player base or player tank
	{
		debugRect.setFillColor(sf::Color(0, 0, 135, 100)); // Set the colour of the debug rectangle to blue
	}
	if (contains == Object::PLAYERSHELL) // If it contains a player shell
	{
		debugRect.setFillColor(sf::Color(0, 100, 50, 100)); // Set the colour of the debug rectangle to green
	}
	if (contains == Object::UNKNOWN) // If it contains unknown
	{
		debugRect.setFillColor(sf::Color(0, 0, 0, 0)); // Set the colour of the debug rectangle to default
	}
	if (bPath) // If it is a path
	{
		debugRect.setFillColor(sf::Color(255, 0, 0, 100)); // Set the debug rect to the path colour
	}
}

void MapNode::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(debugRect); // Draw the debug rectangle
}