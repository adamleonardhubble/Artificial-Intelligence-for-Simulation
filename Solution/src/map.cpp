/*! \file map.cpp
* \brief Source file for the Map class.
*
* Contains the definitions for the Map class' constructor and methods.
*/

#include "map.h"

Map::Map()
{
	sf::Vector2f size = sf::Vector2f(kiBackgroundWidth / s_kiWidth, kiBackgroundHeight / s_kiHeight); // Size of each node
	sf::Vector2f pos = sf::Vector2f(10.f + (size.x / 2.f), 10.f + (size.y / 2.f)); // Position of the first node

	for (int i = 0; i < s_kiWidth; i++)
	{ // For each node
		for (int j = 0; j < s_kiHeight; j++)
		{
			//std::cout << "UNKNOWN: " << i << ", " << j << std::endl;
			// Make a new one in the correct position, start as unknown
			node[i][j] = MapNode(sf::Vector2f(pos.x + (i * size.x), pos.y + (j * size.y)), size, Object::UNKNOWN);
		}
	}

	// Resize the adjacency matrix to the number of nodes by the number of nodes
	vbAdjacencyMatrix.resize(s_kiNodes);
	for (int i = 0; i < s_kiNodes; i++)
	{
		vbAdjacencyMatrix[i].resize(s_kiNodes);
	}

	for (int i = 0; i < s_kiWidth; i++)
	{
		for (int j = 0; j < s_kiHeight; j++)
		{
			// Set the number for each node
			iIndexes[i][j] = (j * s_kiWidth) + i;
		}
	}

	setMapTraversable(); // Set what nodes are traversable
}

void Map::mark(sf::FloatRect objectBounds, Object type)
{
	for (int i = 0; i < s_kiWidth; i++)
	{ // For each node
		for (int j = 0; j < s_kiHeight; j++)
		{
			// If the node intersects with the found object and it's type is currently unknown
			if (node[i][j].getBorder().intersects(objectBounds))
			{
				if (node[i][j].getObjectType() == Object::UNKNOWN)
					node[i][j].updateType(type); // Update the node with it's contained object type
			}
		}
	}
}

void Map::update(int i, int j, bool canSee, Position pos, sf::Vector2i goal)
{
	// If the node can be seen
	if (canSee)
	{
		// If the node has a base in it
		if (node[i][j].getObjectType() == Object::PLAYERBASE || node[i][j].getObjectType() == Object::OWNBASE)
		{
			// If the node was not previously seen and is a path
			if (!node[i][j].bSeen && node[i][j].isPath())
			{
				// Make a new path around it
				makeNewPath(pos.getX(), pos.getY(), goal);
			}
			// If the node was not previously seen and is not a path
			if (!node[i][j].bSeen && !node[i][j].isPath())
			{
				// Update it's type to unknown (If it's been destroyed, it will be traversable again
				node[i][j].updateType(Object::UNKNOWN);
			}
			// It is currently seen
			node[i][j].bSeen = true;
		}

		/*if (node[i][j].getObjectType() == Object::UNKNOWN)
		{
			node[i][j].debugRect.setFillColor(sf::Color(0, 255, 0, 40));
		}*/
	}
	else // If the node can't be seen
	{
		node[i][j].bSeen = false; // It can't be seen

		/*if (node[i][j].getObjectType() == Object::UNKNOWN)
		{
			node[i][j].debugRect.setFillColor(sf::Color(0, 0, 0, 0));
		}*/
	}


	// ************************************************************************************************************************************************************************************************ (J) Added || node[i][j].getObjectType() == Object::PLAYERBASE to solve a problem with tank thinking bases are there when they arent
	// If the node has the player tank or a player shell
	if (node[i][j].getObjectType() == Object::PLAYERSHELL || node[i][j].getObjectType() == Object::PLAYERBASE || node[i][j].getObjectType() == Object::PLAYERTANK)
	{

		// Update it's type to unknown (If it's been destroyed, it will be traversable again
		node[i][j].updateType(Object::UNKNOWN);

	}

	/*if (node[i][j].getObjectType() == Object::PLAYERTANK)
	{
		if (iTankNotVisibleFrameCount >= kiTankNotVisibleFrameMin)
		{
			iTankNotVisibleFrameCount = 0;
			// Update its type to unknown (If it's been destroyed, it will be traversable again
			node[i][j].updateType(Object::UNKNOWN);
		}
		else if (node[i][j].getObjectType() == Object::PLAYERTANK)
		{
			iTankNotVisibleFrameCount++;
		}
	}*/

	/*if (node[i][j].getObjectType() == Object::PLAYERTANK)
	{
		if (!(node[i][j].getObjectType() == Object::PLAYERTANK) || (iTankNotVisibleFrameCount >= kiTankNotVisibleFrameMin))
		{
			iTankNotVisibleFrameCount = 0;
			// Update its type to unknown (If it's been destroyed, it will be traversable again
			node[i][j].updateType(Object::UNKNOWN);
		}
		else if (node[i][j].getObjectType() == Object::PLAYERTANK)
		{
			iTankNotVisibleFrameCount++;
		}
	}*/
		
}

void Map::makeNewPath(float x, float y, sf::Vector2i &goalNode)
{
	int goalX = goalNode.x;
	int goalY = goalNode.y;

	// If there is still a path being followed
	if (!currentPath.empty())
	{
		// Iterate through the path list
		for (std::list<int>::iterator graphListIter = currentPath.begin(); graphListIter != currentPath.end(); ++graphListIter)
		{
			int i;
			int j;
			inverseIndex(*graphListIter, i, j); // Get the nodes map coordinates
			node[i][j].setIfPath(false); // The node is no longer in a path
			node[i][j].resetColour();
		}
		currentPath.clear(); // Clear the path
	}

	int nodeX; // Current node x and y values
	int nodeY;

	for (int i = 0; i < s_kiWidth; i++)
	{ // For each node
		for (int j = 0; j < s_kiHeight; j++)
		{
			// If the node being checked contains the tanks position values
			if (node[i][j].getBorder().contains(sf::Vector2f(x, y)))
			{
				//inverseIndex(index(i, j), nodeX, nodeY);
				nodeX = i;
				nodeY = j;
			}
		}
	}

	// Check area of nodes around possible path if they are traversable
	setAreaTraversable(nodeX, nodeY, goalX, goalY);

	bool bCant = true; // For while loop
	int iNum = 1; // Number for checking
	while (bCant) // While true
	{
		// If the goal node is not traversable
		if (!traversable(node[goalX][goalY].getObjectType()))
		{
			// If there is a node that is traversable iNum number of nodes to the left
			if (goalX > iNum - 1 && traversable(node[goalX - iNum][goalY].getObjectType()))
			{
				goalX -= iNum; // Set that node as the goal node
				goalNode.x -= iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes to the right
			else if (goalX < s_kiWidth - iNum && traversable(node[goalX + iNum][goalY].getObjectType()))
			{
				goalX += iNum; // Set that node as the goal node
				goalNode.x += iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes up
			else if (goalY > iNum - 1 && traversable(node[goalX][goalY - iNum].getObjectType()))
			{
				goalY -= iNum; // Set that node as the goal node
				goalNode.y -= iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes down
			else if (goalY < s_kiHeight - iNum && traversable(node[goalX][goalY + iNum].getObjectType()))
			{
				goalY += iNum; // Set that node as the goal node
				goalNode.y += iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes left and up
			else if (goalX > iNum - 1 && goalY > iNum - 1 && traversable(node[goalX - iNum][goalY - iNum].getObjectType()))
			{
				goalX -= iNum; // Set that node as the goal node
				goalNode.x -= iNum;
				goalY -= iNum; // Set that node as the goal node
				goalNode.y -= iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes left and down
			else if (goalX > iNum - 1 && goalY < s_kiHeight - iNum && traversable(node[goalX - iNum][goalY + iNum].getObjectType()))
			{
				goalX -= iNum; // Set that node as the goal node
				goalNode.x -= iNum;
				goalY += iNum; // Set that node as the goal node
				goalNode.y += iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes right and up
			else if (goalX < s_kiWidth - iNum && goalY > iNum - 1 && traversable(node[goalX + iNum][goalY - iNum].getObjectType()))
			{
				goalX += iNum; // Set that node as the goal node
				goalNode.x += iNum;
				goalY -= iNum; // Set that node as the goal node
				goalNode.y -= iNum;
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes right and down
			else if (goalX < s_kiWidth - iNum && goalY < s_kiHeight - iNum && traversable(node[goalX + iNum][goalY + iNum].getObjectType()))
			{
				goalX += iNum; // Set that node as the goal node
				goalNode.x += iNum;
				goalY += iNum; // Set that node as the goal node
				goalNode.y += iNum;
				bCant = false; // To leave the loop
			}
		}
		else // If the goal node is traversable
			bCant = false; // To leave the loop
		iNum++; // Increase distance of node being checked
	}

	bCant = true;
	iNum = 1;

	while (bCant) // While true
	{
		// If the goal node is not traversable
		if (!traversable(node[nodeX][nodeY].getObjectType()))
		{
			// If there is a node that is traversable iNum number of nodes to the left
			if (nodeX > iNum - 1 && traversable(node[nodeX - iNum][nodeY].getObjectType()))
			{
				nodeX -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes to the right
			else if (nodeX < s_kiWidth - iNum && traversable(node[nodeX + iNum][nodeY].getObjectType()))
			{
				nodeX += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes up
			else if (nodeY > iNum - 1 && traversable(node[nodeX][nodeY - iNum].getObjectType()))
			{
				nodeY -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes down
			else if (nodeY < s_kiHeight - iNum && traversable(node[nodeX][nodeY + iNum].getObjectType()))
			{
				nodeY += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes left and up
			else if (nodeX > iNum - 1 && nodeY > iNum - 1 && traversable(node[nodeX - iNum][nodeY - iNum].getObjectType()))
			{
				nodeX -= iNum; // Set that node as the goal node
				nodeY -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes left and down
			else if (nodeX > iNum - 1 && nodeY < s_kiHeight - iNum && traversable(node[nodeX - iNum][nodeY + iNum].getObjectType()))
			{
				nodeX -= iNum; // Set that node as the goal node
				nodeY += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes right and up
			else if (nodeX < s_kiWidth - iNum && nodeY > iNum - 1 && traversable(node[nodeX + iNum][nodeY - iNum].getObjectType()))
			{
				nodeX += iNum; // Set that node as the goal node
				nodeY -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes right and down
			else if (nodeX < s_kiWidth - iNum && nodeY < s_kiHeight - iNum && traversable(node[nodeX + iNum][nodeY + iNum].getObjectType()))
			{
				nodeX += iNum; // Set that node as the goal node
				nodeY += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
		}
		else // If the goal node is traversable
			bCant = false; // To leave the loop
		iNum++; // Increase distance of node being checked
	}
	// Set the current path to be followed to a new path
	currentPath = aStarSearch(nodeX, nodeY, goalX, goalY);
	//currentPath = dfsSearch(nodeX, nodeY, goalX, goalY);
	//currentPath = bfsSearch(nodeX, nodeY, goalX, goalY);

	// Iterate throught the path list
	for (std::list<int>::iterator graphListIter = currentPath.begin(); graphListIter != currentPath.end(); ++graphListIter)
	{
		int i;
		int j;
		inverseIndex(*graphListIter, i, j); // Get the nodes map coordinates
		node[i][j].setIfPath(true); // The node is now in a path
	}
}

sf::Vector2f Map::followPath(Position pos)
{
	// If there is no path to follow
	if (currentPath.empty())
	{
		// Tell tank to move to it's current position
		return sf::Vector2f(pos.getX(), pos.getY());
	}

	// Index value of the node at the front of the path list
	int nextNode = currentPath.front();
	// Nodes x value
	int nodeX;
	int nodeY; // Nodes y value
	// Get the nodes x and y values based on the index value
	inverseIndex(nextNode, nodeX, nodeY);
	sf::FloatRect nodeBorder = node[nodeX][nodeY].getBorder();
	sf::Vector2f nodeWorldPos = sf::Vector2f(nodeBorder.left + (nodeBorder.width / 2.f), nodeBorder.top + (nodeBorder.height / 2.f));

	// If the node contains the tanks current position
	if (/*node[nodeX][nodeY].getBorder().contains(sf::Vector2f(pos.getX(), pos.getY()))*/ pos.getX() > nodeWorldPos.x - 1.75f && pos.getX() < nodeWorldPos.x + 1.75f && pos.getY() > nodeWorldPos.y - 1.75f && pos.getY() < nodeWorldPos.y + 1.75f)
	{
		node[nodeX][nodeY].setIfPath(false);
		node[nodeX][nodeY].updateType(Object::UNKNOWN);
		currentPath.pop_front(); // Remove the front node from the path list
		if (!currentPath.empty()) // If there is still path to follow
			nextNode = currentPath.front(); // Set the current value to the new front of the path list
	}
	// The border of the node being moved to
	//sf::FloatRect nodeBorder = node[nodeX][nodeY].getBorder();
	// The global position of the node being moved to
	//sf::Vector2f nodePos = sf::Vector2f(nodeBorder.left + (nodeBorder.width / 2.f), nodeBorder.top + (nodeBorder.height / 2.f));
	// Return the global position of the node being moved to
	return nodeWorldPos;
}

sf::FloatRect Map::getNodeBox(int i, int j)
{
	// Return the nodes floatrect
	return node[i][j].getBorder();
}

Object Map::getNodeObject(int i, int j)
{
	// Return the nodes object type
	return node[i][j].getObjectType();
}

bool Map::traversable(Object type)
{
	// Return true if the nodes type is unknown
	return type == Object::UNKNOWN || type == Object::PLAYERSHELL;
}

int Map::index(int x, int y)
{
	// Return the number for the node
	return iIndexes[x][y];
}

void Map::inverseIndex(int index, int& x, int& y)
{
	// If the index value is out of range
	if (index > s_kiNodes)
	{
		// Do nothing
		return;
	}
	else // If not
	{
		// Set the x and y values to that of the node with the passed in index
		x = index % s_kiWidth;
		y = index / s_kiWidth;
	}
}

void Map::setMapTraversable()
{
	// Call function to check whole map if traversable
	setAreaTraversable(0, 0, s_kiWidth, s_kiHeight);

	/*for (int i = 0; i < s_kiNodes; i++)
	{
		for (int j = 0; j < s_kiNodes; j++)
		{
			// Set all values to false as a reset
			vbAdjacencyMatrix[i][j] = false;
		}
	}

	for (int i = 0; i < s_kiWidth; i++)
	{ // For all nodes
		for (int j = 0; j < s_kiHeight; j++)
		{
			// If the node is traversable
			if (traversable(node[i][j].getObjectType()))
			{
				// Get a value for the node
				int position = (j * s_kiWidth) + i;
				// If the node is not at the far left
				if (i > 0)
				{
					// If the node to the left is traversable
					if (traversable(node[i - 1][j].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position - 1] = true;
					}
				}
				// If the node is not at the far right
				if (i < s_kiWidth - 1)
				{
					// If the node to the right is traversable
					if (traversable(node[i + 1][j].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position + 1] = true;
					}
				}
				// If the node is not at the top
				if (j > 0)
				{
					// If the node above is traversable
					if (traversable(node[i][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position - s_kiWidth] = true;
					}
				}
				// If the node is not at the bottom
				if (j < s_kiHeight - 1)
				{
					// If the node below is traversable
					if (traversable(node[i][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position + s_kiWidth] = true;
					}
				}
				// If the node is not at the far left and not at the top
				if (i > 0 && j > 0)
				{
					if (traversable(node[i - 1][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position - s_kiWidth) - 1] = true;
					}
				}
				// If the node is not at the far left and not at the bottom
				if (i > 0 && j < s_kiHeight - 1)
				{
					if (traversable(node[i - 1][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position + s_kiWidth) - 1] = true;
					}
				}
				// If the node is not at the far right and not at the top
				if (i < s_kiWidth - 1 && j > 0)
				{
					if (traversable(node[i + 1][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position - s_kiWidth) + 1] = true;
					}
				}
				// If the node is not at the far right and not at the bottom
				if (i < s_kiWidth - 1 && j < s_kiHeight - 1)
				{
					if (traversable(node[i + 1][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position + s_kiWidth) + 1] = true;
					}
				}
			}
		}
	}*/
}

void Map::setAreaTraversable(int x1, int y1, int x2, int y2)
{
	int startX; // Start node x and y
	int startY;
	int endX; // End node x and y
	int endY;
	int startNode; // Index value of start node
	int nodes; // Index value for the end node

	int extraSpace = 4; // To increase the area checked

	// For the lower x, set it to start x and take extra space off. For the higher x, set it to end and add extra space on
	if (x1 < x2)
	{
		startX = x1 - extraSpace;
		endX = x2 + extraSpace;
	}
	else if (x1 > x2)
	{
		startX = x2 - extraSpace;
		endX = x1 + extraSpace;
	}
	else // If they are the same
	{
		// Set start to the x value - extra space
		startX = x1 - extraSpace;
		// Set end to the x value + extra space
		endX = x1 + extraSpace;
	}
	// Same for y as done above for x
	if (y1 < y2)
	{
		startY = y1 - extraSpace;
		endY = y2 + extraSpace;
	}
	else if (y1 > y2)
	{
		startY = y2 - extraSpace;
		endY = y1 + extraSpace;
	}
	else
	{
		startY = y1 - extraSpace;
		endY = y1 + extraSpace;
	}
	// If the start and end x and y values are out of range, make them in range
	if (startX < 0)
		startX = 0;
	if (startY < 0)
		startY = 0;
	if (endX > s_kiWidth - 1)
		endX = s_kiWidth - 1;
	if (endY > s_kiHeight - 1)
		endY = s_kiHeight - 1;
	// Get the index value of the start node
	startNode = index(startX, startY);
	nodes = index(endX, endY); // Get the index value of the end node

	for (int i = startNode; i <= nodes; i++)
	{
		for (int j = startNode; j <= nodes; j++)
		{
			// Set all values being checked to false as a reset
			vbAdjacencyMatrix[i][j] = false;
		}
	}

	for (int i = startX; i <= endX; i++)
	{ // For all nodes being checked
		for (int j = startY; j <= endY; j++)
		{
			// If the node is traversable
			if (traversable(node[i][j].getObjectType()))
			{
				// Get a value for the node
				int position = (j * s_kiWidth) + i;
				// If the node is not at the far left
				if (i > 0)
				{
					// If the node to the left is traversable
					if (traversable(node[i - 1][j].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position - 1] = true;
					}
				}
				// If the node is not at the far right
				if (i < s_kiWidth - 1)
				{
					// If the node to the right is traversable
					if (traversable(node[i + 1][j].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position + 1] = true;
					}
				}
				// If the node is not at the top
				if (j > 0)
				{
					// If the node above is traversable
					if (traversable(node[i][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position - s_kiWidth] = true;
					}
				}
				// If the node is not at the bottom
				if (j < s_kiHeight - 1)
				{
					// If the node below is traversable
					if (traversable(node[i][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][position + s_kiWidth] = true;
					}
				}
				// If the node is not at the far left and not at the top
				if (i > 0 && j > 0)
				{
					// If the node to the top left, the node above and the node to the left are traversable
					if (traversable(node[i - 1][j - 1].getObjectType()) &&
						traversable(node[i - 1][j].getObjectType()) &&
						traversable(node[i][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position - s_kiWidth) - 1] = true;
					}
				}
				// If the node is not at the far left and not at the bottom
				if (i > 0 && j < s_kiHeight - 1)
				{
					// If the node to the bottom left, the node below and the node to the left are traversable
					if (traversable(node[i - 1][j + 1].getObjectType()) &&
						traversable(node[i - 1][j].getObjectType()) &&
						traversable(node[i][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position + s_kiWidth) - 1] = true;
					}
				}
				// If the node is not at the far right and not at the top
				if (i < s_kiWidth - 1 && j > 0)
				{
					// If the node to the top right, the node above and the node to the right are traversable
					if (traversable(node[i + 1][j - 1].getObjectType()) &&
						traversable(node[i + 1][j].getObjectType()) &&
						traversable(node[i][j - 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position - s_kiWidth) + 1] = true;
					}
				}
				// If the node is not at the far right and not at the bottom
				if (i < s_kiWidth - 1 && j < s_kiHeight - 1)
				{
					// If the node to the bottom right, the node below and the node to the right are traversable
					if (traversable(node[i + 1][j + 1].getObjectType()) &&
						traversable(node[i + 1][j].getObjectType()) &&
						traversable(node[i][j + 1].getObjectType()))
					{
						// Set that in the adjacency matrix
						vbAdjacencyMatrix[position][(position + s_kiWidth) + 1] = true;
					}
				}
			}
		}
	}
}

std::list<int> Map::aStarSearch(int currentX, int currentY, int goalX, int goalY)
{
	std::list<MapNode> open; // Nodes not checked
	std::list<MapNode> closed; // Nodes that have been checked

	int goal; // The goal nodes value
	int current; // The current nodes value
	MapNode currentNode; // The current node

	goal = index(goalX, goalY); // Set the goal node value
	current = index(currentX, currentY); // Set the current node value

	currentNode.iIndex = current; // Set the index value of the current node
	currentNode.iParentIndex = -1; // Set the parent index value of the current node to nothing as there is no previous node
	currentNode.score(0.0, currentX, currentY, goalX, goalY); // Set the score of the current node

	open.push_back(currentNode); // Put the current node on the open list

	while (current != goal) // Repeat until reached goal node
	{
		open.sort(); // Sort the open list
		// Set the current node to the front node in the open list
		currentNode = open.front();
		open.pop_front(); // Remove the front node from the list
		closed.push_back(currentNode); // Add the current node to the back of the closed list
		current = currentNode.iIndex; // Set the value of the current node to the current nodes index value

		// For every node
		for (int other = 0; other < s_kiNodes; other++)
		{
			// If current node to other node is traversable
			if (vbAdjacencyMatrix[current][other])
			{
				std::list<MapNode>::iterator graphListIter; // Iterator to step through list
				bool onClosed = false; // If the node is on the closed list
				bool onOpen = false; // If the node is on the open list
				// For every node in the closed list
				for (graphListIter = closed.begin(); graphListIter != closed.end(); ++graphListIter)
				{
					// If the node in the closed list is that same as the other node
					if ((*graphListIter).iIndex == other)
					{
						onClosed = true; // It is on the closed list
						// If the geographical score of the current node is less than that of the node in the list
						if (currentNode.fGeogScore + 1.0f < (*graphListIter).fGeogScore)
						{
							// Set the current value to the parent index value of the node in the list
							(*graphListIter).iParentIndex = current;
						}
					}
				}
				// If the node is not on the closed list
				if (!onClosed)
				{
					// For every node in the open list
					for (graphListIter = open.begin(); graphListIter != open.end(); ++graphListIter)
					{
						// If the node in the open list is that same as the other node
						if ((*graphListIter).iIndex == other)
						{
							onOpen = true; // It is on the open list
							// If the geographical score of the current node is less than that of the node in the list
							if (currentNode.fGeogScore + 1.0f < (*graphListIter).fGeogScore)
							{
								// Set the current value to the parent index value of the node in the list
								(*graphListIter).iParentIndex = current;
							}
						}
					}
				}
				// If the node is not on either list
				if (!onClosed && !onOpen)
				{
					MapNode tmp; // Temporary node
					// Set the parent index value to the current value
					tmp.iParentIndex = current;
					tmp.iIndex = other; // Set the index value to the other value
					inverseIndex(other, currentX, currentY); // Get the x and y values of the node using the other value
					// Score the temporary node
					tmp.score(currentNode.fGeogScore, currentX, currentY, goalX, goalY);
					open.push_back(tmp); // Put it on the back of the open list
				}
			}
		}
	}

	return buildPath(closed); // Return the path list
}

std::list<int> Map::dfsSearch(int currentX, int currentY, int goalX, int goalY)
{
	std::list<MapNode> closed; // Nodes that have been checked
	std::stack<MapNode> nodeStack; // Nodes to check
	bool bVisited[s_kiNodes] = { false }; // If the node has been checked

	int goal; // Index value of goal node
	int current; // Index value of current node
	MapNode currentNode; // The current node

	goal = index(goalX, goalY); // Get the index values
	current = index(currentX, currentY);
	bVisited[current] = true; // Current has been checked

	currentNode.iIndex = current; // Set the current nodes index value to the current index value
	currentNode.iParentIndex = -1; // Set the parent index value of the current node to nothing as there is no previous node

	nodeStack.push(currentNode); // Put the current node on the stack
	// While hasn't reached the goal node
	while (current != goal)
	{
		// Current is the top node in the stack
		currentNode = nodeStack.top();
		nodeStack.pop(); // Remove the top node in the stack
		// Add current to the back of the closed list
		closed.push_back(currentNode);
		current = currentNode.iIndex; // Get the current nodes index value
		// For each node
		for (int other = 0; other < s_kiNodes; other++)
		{
			// If can move from current to other nodes
			if (vbAdjacencyMatrix[current][other])
			{
				// If other hasn't been checked
				if (!bVisited[other])
				{
					bVisited[other] = true; // Set it so it has been visited

					MapNode tmp; // Temporary node
					// Set the parent index value to the current value
					tmp.iParentIndex = current;
					tmp.iIndex = other; // Set the index value to the other value
					inverseIndex(other, currentX, currentY); // Get the x and y values of the node using the other value
					nodeStack.push(tmp); // Add the temporary node to the stack
				}
			}
		}
	}
	// Rebuild the path and return it
	return buildPath(closed);
}

std::list<int> Map::bfsSearch(int currentX, int currentY, int goalX, int goalY)
{
	std::list<MapNode> open; // Nodes to be checked
	std::list<MapNode> closed; // Nodes that have been checked
	bool bVisited[s_kiNodes] = { false }; // If the node has been checked

	int goal; // Index value of goal node
	int current; // Index value of current node
	MapNode currentNode; // The current node

	goal = index(goalX, goalY); // Get the index values
	current = index(currentX, currentY);
	bVisited[current] = true; // Current has been checked

	currentNode.iIndex = current; // Set the current nodes index value to the current index value
	currentNode.iParentIndex = -1; // Set the parent index value of the current node to nothing as there is no previous node

	open.push_back(currentNode); // Put the current node on the list
	// While hasn't reached the goal node
	while (current != goal)
	{
		// Current is the front node in the open list
		currentNode = open.front();
		open.pop_front(); // Remove the front node in the open list
		// Add current to the back of the closed list
		closed.push_back(currentNode);
		current = currentNode.iIndex; // Get the current nodes index value
		// For each node
		for (int other = 0; other < s_kiNodes; other++)
		{
			// If can move from current to other nodes
			if (vbAdjacencyMatrix[current][other])
			{
				// If other hasn't been checked
				if (!bVisited[other])
				{
					bVisited[other] = true; // Set it so it has been visited

					MapNode tmp; // Temporary node
					// Set the parent index value to the current value
					tmp.iParentIndex = current;
					tmp.iIndex = other; // Set the index value to the other value
					inverseIndex(other, currentX, currentY); // Get the x and y values of the node using the other value
					open.push_back(tmp); // Add the temporary node to the back of the open list
				}
			}
		}
	}
	// Rebuild the path and return it
	return buildPath(closed);
}

std::list<int> Map::buildPath(std::list<MapNode>& searched)
{
	// Path to return
	std::list<int> path;
	int parent; // Index value of the parent node
	std::list<MapNode>::iterator graphListIter; // To iterate through searched list
	// Set the current node to the back node of the searched list
	MapNode currentNode = searched.back();
	parent = currentNode.iParentIndex; // Set parent to the parent index value of the current node
	path.push_front(currentNode.iIndex); // Add the current index value to the path list
	searched.pop_back(); // Remove the back node from the searched list
	// Iterate through the searched list
	for (graphListIter = searched.end(), --graphListIter; graphListIter != searched.begin(); --graphListIter)
	{
		// Set current node to the current node being checked in searched
		currentNode = *graphListIter;
		// If the current nodes parent index value is the same as the parent value
		if (currentNode.iIndex == parent)
		{
			// Add the parent value to the front of the path list
			path.push_front(parent);
			// Set the parent value to the current nodes parent index value
			parent = currentNode.iParentIndex;
			searched.erase(graphListIter); // Remove the current node being checked from the searched list
			graphListIter = searched.end(); // Reset the iterator
		}
	}
	// Return the path
	return path;
}

void Map::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	for (int i = 0; i < s_kiWidth; i++)
	{
		for (int j = 0; j < s_kiHeight; j++)
		{
			// Draw each node
			target.draw(node[i][j]);
		}
	}
}