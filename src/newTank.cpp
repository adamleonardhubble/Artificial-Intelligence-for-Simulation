/*! \file newTank.cpp
* \brief Source file for the NewTank class.
*
* Contains the definitions for the NewTank class' constructor and methods.
*/

#include "newTank.h"

NewTank::NewTank()
{
	// Setting size and origin for player shell path, used for dodging
	for (int i = 0; i < s_kiNumRectShellPath; i++)
	{
		shellPath[i].setSize(sf::Vector2f(kfShellPathExtent, kfShellPathExtent));
		shellPath[i].setOrigin(sf::Vector2f(kfShellPathExtent/2, kfShellPathExtent/2));
	}

	// Default position of closest enemy when no enemy is seen, preventing it aiming towards something not there
	closestEnemyPos = noEnemySeenPos;

	// Default states
	iMovementState = AIMovementStates::SEARCHING;
	iWeaponState = AIWeaponStates::SEARCHINGAIM;

	// Resetting tank
	reset();
}

void NewTank::setTargetInfo()
{
	// Vector distance between AI tank and target
	closestEnemyPosDiff = (sf::Vector2f(getX(), getY()) - closestEnemyPos);

	// Calculates the clockwise angle between the AI tank and the closest enemy base or player in degrees, where directly right is 0 degrees
	fAngleDiff = calcAngle(closestEnemyPosDiff) + 90.f;

	// Calculates scalar distance to the target
	fDistanceToTarget = sqrt(pow((getX() - closestEnemyPos.x), 2) + pow((getY() - closestEnemyPos.y), 2));

	// Current node
	currentNode = calcNodePos(getX(), getY());
}

void NewTank::checkVision()
{
	// Resets bools for vision
	bCanSeeEnemyTank = false;
	bCanSeeEnemyBase = false;

	// If the AI tank can see an enemy player or base, it will set the bools to true, checks every node in map
	for (int i = 0; i < map.getWidth(); i++)
	{
		for (int j = 0; j < map.getHeight(); j++)
		{
			BoundingBox box; // To use to check if the node is viewable by the enemy tank
			sf::FloatRect nodeBox = map.getNodeBox(i, j); // Get the floatrect for the node

			box.set(nodeBox.left, nodeBox.top, nodeBox.left + nodeBox.width, nodeBox.top + nodeBox.height); // Set the bounding box for the area to check

			// If can see an enemy tank
			if (canSee(box) && (map.getNodeObject(i, j) == Object::PLAYERTANK))
			{
				bCanSeeEnemyTank = true;
			}

			// If can see an enemy base
			if (canSee(box) && (map.getNodeObject(i, j) == Object::PLAYERBASE))
			{
				bCanSeeEnemyBase = true;
				//std::cout << "base is visible" << std::endl;
			}
		}
	}

	if (!bCanSeeEnemyTank)
	{
		iLostPlayerTankFrames++;
	}
	else
	{
		iLostPlayerTankFrames = 0;
	}
	//std::cout << iLostPlayerTankFrames << std::endl;
}

void NewTank::setStateMachineConditions()
{
	// Higher priority states go before lower priority states. See newTank.h for priority list.

	// If in a seen shell path's node, dodge.
	if (map.getNodeObject(currentNode.x, currentNode.y) == Object::PLAYERSHELL)
	{
		// Sets movement state to dodging
		iMovementState = AIMovementStates::DODGING;
	}

	// If the AI tank can see an enemy tank and has no ammo, attempt to escape to another corner of the map.
	else if ((bCanSeeEnemyTank && !hasAmmo()) || ((currentNode != goalNode) && (iMovementState == AIMovementStates::ESCAPING)))
	{
		// Sets movement state to escaping
		iMovementState = AIMovementStates::ESCAPING;
	}

	// If the AI tank has no ammo, attempt to hide in the corner of the map.
	else if ((!hasAmmo() && (iMovementState != AIMovementStates::ESCAPING)) || ((iMovementState == AIMovementStates::ESCAPING) && (goalNode == currentNode)))
	{
		// Sets movement state to hiding
		iMovementState = AIMovementStates::HIDING;
	}

	// If the AI tank can see an enemy tank or enemy base, stop and shoot at it. Rotates perpendicular to the enemy object if it is a player tank.
	else if ((bCanSeeEnemyTank || bCanSeeEnemyBase) && ((!(iMovementState == AIMovementStates::FOLLOWING)) || (iFollowingFrameCount > kiFollowingMinFrameCount)))
	{
		// Sets movement state to stopping
		iMovementState = AIMovementStates::STOPPING;
	}

	// If an enemy has left the AI tank's vision, will check last seen location. Leaves state when it reaches the last seen location.
	else if (((iMovementState == AIMovementStates::STOPPING) || (iMovementState == AIMovementStates::FOLLOWING)) && !(bCanSeeEnemyTank) && !(bCanSeeEnemyBase) && (goalNode != currentNode))
	{
		// Sets movement state to following
		if (iLostPlayerTankFrames >= kiLostPlayerMinFrameCount)
		{
			iMovementState = AIMovementStates::FOLLOWING;
		}

		// To prevent lag if rapid transitioning between FOLLOWING and STOPPING states.
		else
		{
			iMovementState = AIMovementStates::STOPPING;
		}
	}

	else
	{
		// Default state, tank searches around the map randomly
		iMovementState = AIMovementStates::SEARCHING;
	}
}

void NewTank::moveStateMachineActions()
{
	// AI tank calculates goal nodes and moves depending on its state here.
	switch (iMovementState)
	{

	// If dodging
	case DODGING:

		// Search with turret
		iWeaponState = AIWeaponStates::SEARCHINGAIM;

		// If first frame of dodging, makes tank dodge forward. Dodging doesn't calculate a new path, as the tank would not have enough time to dodge a projectile if it did so.
		if (iMovementState != iPrevMovementState)
		{
			goForward();

			// std::cout << "Enemy projectile inbound, dodging." << std::endl;
		}
		break;

	// If escaping
	case ESCAPING:

		// Search with turret
		iWeaponState = AIWeaponStates::SEARCHINGAIM;

		// If first frame of escaping or is on its goal node, calculate new path to the next corner in the map.
		if ((iMovementState != iPrevMovementState) || (goalNode == currentNode))
		{
			// If at top left corner of map, escape to bottom left corner.
			if ((currentNode.x < floor((map.getWidth() / 2) - 1)) && (currentNode.y < floor((map.getHeight() / 2) - 1)))
			{
				goalNode = sf::Vector2i(1, map.getHeight() - 2);
			}

			// If at top right corner of the map, escape to top left corner.
			if ((currentNode.x >= floor(map.getWidth() / 2)) && (currentNode.y < floor((map.getHeight() / 2) - 1)))
			{
				goalNode = sf::Vector2i(1, 1);
			}

			// If at bottom right corner of the map, escape to top right corner.
			if ((currentNode.x >= floor(map.getWidth() / 2)) && (currentNode.y >= floor(map.getHeight() / 2)))
			{
				goalNode = sf::Vector2i(map.getWidth() - 2, 1);
			}

			// If at bottom left corner of the map, escape to bottom right corner.
			if ((currentNode.x < floor((map.getWidth() / 2) - 1)) && (currentNode.y >= floor(map.getHeight() / 2)))
			{
				goalNode = sf::Vector2i(map.getWidth() - 2, map.getHeight() - 2);
			}


			// Random node on map away from player is escape destination.
			/*if ((currentNode.x < calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x) && (currentNode.y < calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y))
			{
				goalNode = sf::Vector2i(rand() % currentNode.x + 1, rand() % currentNode.y + 1);
				std::cout << "Running to top left" << std::endl;
			}
			else if ((currentNode.x >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x) && (currentNode.y < calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y))
			{
				goalNode = sf::Vector2i((map.getWidth() - currentNode.x + 1), rand() % currentNode.y + 1);
				std::cout << "Running top right" << std::endl;
			}
			else if ((currentNode.x >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x) && (currentNode.y >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y))
			{
				goalNode = sf::Vector2i((rand() % (map.getWidth() - currentNode.x + 1)) + currentNode.x, (rand() % (map.getHeight() - currentNode.y + 1)) + currentNode.y);
				std::cout << "Running to bottom right" << std::endl;
			}
			else if ((currentNode.x < calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x) && (currentNode.y >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y))
			{
				goalNode = sf::Vector2i(rand() % currentNode.x + 1, (map.getHeight() - currentNode.y + 1));
				std::cout << "Running to bottom left" << std::endl;
			}
			else
			{
				std::cout << "Error" << std::endl;
			}*/

			// std::cout << "Player spotted and there is no ammo, escaping." << std::endl;
		}
		break;

	// If hiding
	case HIDING:

		// Search with turret to find potential threats
		iWeaponState = AIWeaponStates::SEARCHINGAIM;

		// If first frame of hiding, calculate new path.
		if (iMovementState != iPrevMovementState)
		{
			// Finds closest corner of the map to the tank, and sets the goal node to that corner.
			// Path to top left corner
			if ((currentNode.x < floor((map.getWidth() / 2) - 1)) && (currentNode.y < floor((map.getHeight() / 2) - 1)))
			{
				goalNode = sf::Vector2i(1, 1);
			}

			// Path to top right corner
			if ((currentNode.x >= floor(map.getWidth() / 2)) && (currentNode.y < floor((map.getHeight() / 2) - 1)))
			{
				goalNode = sf::Vector2i(map.getWidth() - 2, 1);
			}

			// Path to bottom right corner
			if ((currentNode.x >= floor(map.getWidth() / 2)) && (currentNode.y >= floor(map.getHeight() / 2)))
			{
				goalNode = sf::Vector2i(map.getWidth() - 2, map.getHeight() - 2);
			}

			// Path to bottom left corner
			if ((currentNode.x < floor((map.getWidth() / 2) - 1)) && (currentNode.y >= floor(map.getHeight() / 2)))
			{
				goalNode = sf::Vector2i(1, map.getHeight() - 2);
			}
			// std::cout << "No ammo left, hiding." << std::endl;
		}

		// If at goal node, stops tank from moving and rotating.
		if (goalNode == currentNode)
		{
			stop();
		}

		break;

	// If stopping
	case STOPPING:
	{
		// Aim at closest target with turret
		iWeaponState = AIWeaponStates::AIMING;

		// Resets following frame count to 0
		iFollowingFrameCount = 0;

		// Rotate 90 degrees perpendicular to target to aid in dodging
		float fDodgeAngle = calcAngle(sf::Vector2f(pos.getX(), pos.getY()) - closestEnemyPos) + 180.f;

		// Sort it out
		if (fDodgeAngle >= 360)
		{
			fDodgeAngle -= 360;
		}

		// If not angled the right way, and its a player tank that is being aimed at
		if ((!(fDodgeAngle >= pos.getTh() - 1.75f && fDodgeAngle <= pos.getTh() + 1.75f)) && (iClosestEnemyObject == Object::PLAYERTANK)) // If not facing the correct way
		{
			// Rotates tank left until perpendicular
			if (fDodgeAngle < pos.getTh())
			{
				goLeft(); // Turn left
				//std::cout << "Turning left" << std::endl;
			}

			// Rotates tank right until perpendicular
			if (fDodgeAngle > pos.getTh())
			{
				goRight(); // Turn right
			}

			// Same thing but for going across the 359-0 direction
			if (pos.getTh() < 90 && fDodgeAngle > 270)
			{
				goLeft();
				//std::cout << "Turning left" << std::endl;
			}

			if (pos.getTh() > 270 && fDodgeAngle < 90)
			{
				goRight();
			}
		}
		else
		{
			// Stop turning
			left = false;
			right = false;
		}

		// If first frame of stopping, calculate new path to enemy location, which will be used in the FOLLOWING state.
		if (iMovementState != iPrevMovementState)
		{
			goalNode = calcNodePos(closestEnemyPos.x, closestEnemyPos.y);
			// std::cout << "Enemy object spotted, stopping and aiming." << std::endl;
		}

		// If target to follow is not in the goal node any more, calculate new path to the new enemy location.
		if (closestEnemyPos != noEnemySeenPos)
		{
			if (goalNode.x <= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x - 2
				|| goalNode.x >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).x + 2
				|| goalNode.y <= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y - 2
				|| goalNode.y >= calcNodePos(closestEnemyPos.x, closestEnemyPos.y).y + 2)
			{
				goalNode = calcNodePos(closestEnemyPos.x, closestEnemyPos.y);
				// std::cout << "Enemy object spotted, stopping and aiming." << std::endl;
			}
		}
	}
	break;
	
	// If following
	case FOLLOWING:

		// Search with turret
		iWeaponState = AIWeaponStates::SEARCHINGAIM;

		// Increments FOLLOWING frame count
		iFollowingFrameCount++;

		// Used for debugging
		/*if (iMovementState != iPrevMovementState)
		{
			//goalNode = calcNodePos(closestEnemyPos.x, closestEnemyPos.y);
			// std::cout << "Enemy object lost, searching at last known location." << std::endl;
		}*/

		break;

	// If searching
	case SEARCHING:

		// Search with turret
		iWeaponState = AIWeaponStates::SEARCHINGAIM;

		// If first frame of searching or reached the goal node, calculate a new path to random node on the map
		if ((iMovementState != iPrevMovementState) || (currentNode == goalNode))
		{
			goalNode = sf::Vector2i(rand() % (map.getWidth() / 2), rand() % map.getHeight());
			// std::cout << "Searching for enemies." << std::endl;
		}
		break;
	}

	// If state or goal node has changed
	if (((iPrevMovementState != iMovementState) || (goalNode != prevGoalNode)) && (goalNode != currentNode) && (iMovementState != AIMovementStates::DODGING))
	{
		//std::cout << goalNode.x << ", " << goalNode.y;

		// Calculate new path to new goal node
		map.makeNewPath(pos.getX(), pos.getY(), goalNode);
	}

	// Checks if tank is stuck when it should be moving, making it calculate a new path next frame to avoid object it is stuck on.
	checkStuck();

	// Resets previous variables that are used to check for differences in states.
	iPrevMovementState = iMovementState;
	prevNode = currentNode;
	prevGoalNode = goalNode;
}

void NewTank::turretStateMachineActions()
{

	// Depending on weapon state, aims or searches with turret
	switch (iWeaponState)
	{
	
	// If aiming
	case AIMING:

		// Prevents firing whilst aiming and not aimed
		if (bFiring)
		{
			bFiring = false;
		}

		// Rotates turret towards target
		//comparePositions(sf::Vector2f(getX(), getY()),)
		aimTurret();

		// If aimed correctly, fire turret
		if (((fAngleDiff >= turretTh - 1) && (fAngleDiff <= turretTh + 1) && fDistanceToTarget < kfViewDistance) && (!bFiring))
		{
			bFiring = true;
		}
		break;

	// If searching
	case SEARCHINGAIM:

		// Prevents firing whilst searching
		if (bFiring)
		{
			bFiring = false;
		}

		if (left)
		{
			iLeftFrames++;
			iRightFrames = 0;
		}

		else if (right)
		{
			iRightFrames++;
			iLeftFrames = 0;
		}

		// Rotates turret left if tank is turning left, allowing for more vision to be covered in smaller amount of time.
		if (iLeftFrames > kiTurningMinFrameCount)
		{
			turretGoLeft();
		}

		// Rotates turret right if tank is turning right, allowing for more vision to be covered in smaller amount of time.
		if (iRightFrames > kiTurningMinFrameCount)
		{
			turretGoRight();
		}

		// Makes turret turn left if it is not turning
		if (!turretLeft && !turretRight)
		{
			turretGoLeft();
			iLeftFrames = 0;
			iRightFrames = 0;
		}
		break;
	}
}

void NewTank::aimTurret()
{
	// Calculates the difference in position between the closest enemy base or player, and the AI tank
	//sf::Vector2f closestEnemyPosDiff = (sf::Vector2f(getX(), getY()) - closestEnemyPos);

	// Calculates the clockwise angle between the AI tank and the closest enemy base or player in degrees, where directly right is 0 degrees
	//float fAngle = calcAngle(closestEnemyPosDiff) + 90.f;
	// Sort it out
	//std::cout << closestEnemyPos.x << ", " << closestEnemyPos.y << std::endl;

	// Fixes angles over 360
	if (fAngleDiff >= 360)
	{
		fAngleDiff -= 360;
	}

	// If needs to aim left
	if (fAngleDiff < turretTh)
	{
		turretGoLeft(); // Go left
	}

	// If needs to aim right
	if (fAngleDiff > turretTh)
	{
		turretGoRight(); // Go right
	}

	// Same thing but for going across the 359-0 direction
	if (turretTh < 90 && fAngleDiff > 270)
	{
		turretGoLeft();
	}

	if (turretTh > 270 && fAngleDiff < 90)
	{
		turretGoRight();
	}
}

void NewTank::move()
{
	// Sets variables related to a target's location (Their position, the difference in position between the tank and the target and the angle between the tank and the target)
	setTargetInfo();

	// Checks if tank can see any enemy objects
	checkVision();

	// Tests conditions to put the tank in the right movement and weapon states
	setStateMachineConditions();
	
	// Perform movement state actions
	moveStateMachineActions();

	// Perform weapon state actions
	turretStateMachineActions();

	// Path to follow
	sf::Vector2f newPos;

	// Get new position to move to from the path as long as it isn't meant to be stopping/dodging
	if ((iMovementState != AIMovementStates::STOPPING) && (iMovementState != AIMovementStates::DODGING))
	{
		// Follows path
		newPos = map.followPath(pos);
	}

	else
	{
		// Prevents movement of tank if STOPPING
		if ((iMovementState != AIMovementStates::DODGING) && (!bDodging))
		{
			forward = false;
		}
		
		// Stops backward movement
		backward = false;
	}

	// If there is a path to follow
	if ((newPos != sf::Vector2f(pos.getX(), pos.getY())) && (iMovementState != AIMovementStates::STOPPING) && (iMovementState != AIMovementStates::DODGING))
	{
		// Find the target angle for the tank
		float fNewAngle = calcAngle(sf::Vector2f(pos.getX(), pos.getY()) - newPos) + 90.f;
		// Sort it out
		if (fNewAngle >= 360)
		{
			fNewAngle -= 360;
		}
		// If facing the correct way
		if (fNewAngle >= pos.getTh() - kfTurretAccuracyLimit && fNewAngle <= pos.getTh() + kfTurretAccuracyLimit)
		{
			goForward(); // Move forwards
		}
		else // If not facing the correct way
		{
			// If needs to turn left
			if (fNewAngle < pos.getTh())
			{
				goLeft(); // Turn left
			}
			// If needs to turn right
			if (fNewAngle > pos.getTh())
			{
				goRight(); // Turn right
			}
			// Same thing but for going across the 359-0 direction
			if (pos.getTh() < 90.f && fNewAngle > 270.f)
			{
				goLeft();
				//std::cout << "Left: " << pos.getTh() << ", " << fNewAngle << std::endl;
			}
			if (pos.getTh() > 270.f && fNewAngle < 90.f)
			{
				goRight();
				//std::cout << "Right: " << pos.getTh() << ", " << fNewAngle << std::endl;
			}
			if ((pos.getTh() < kfBodyAccuracyLimit && fNewAngle > 360.f - kfBodyAccuracyLimit) || (pos.getTh() > 360.f - kfBodyAccuracyLimit && fNewAngle < kfBodyAccuracyLimit))
			{
				right = false;
				left = false;
				goForward();
			}
			//if (pos.getTh())
		}
	}
	else // If no path to follow
	{
		if ((iMovementState != AIMovementStates::DODGING) && (!bDodging))
		{
			forward = false; // Don't move
		}
		backward = false;
	}

	for (int i = 0; i < map.getWidth(); i++)
	{ // For each node in the map
		for (int j = 0; j < map.getHeight(); j++)
		{
			BoundingBox box; // To use to check if the node is viewable by the enemy tank
			sf::FloatRect nodeBox = map.getNodeBox(i, j); // Get the floatrect for the node

			box.set(nodeBox.left, nodeBox.top, nodeBox.left + nodeBox.width, nodeBox.top + nodeBox.height); // Set the bounding box for the area to check

			map.update(i, j, canSee(box), pos, goalNode); // Update the map

			/*BoundingBox boxTopLeft, boxTopRight, boxBottomLeft, boxBottomRight;
			sf::FloatRect nodeBox = map.getNodeBox(i, j);

			boxTopLeft.set(nodeBox.left, nodeBox.top, 1.f, 1.f);
			boxTopRight.set(nodeBox.left + nodeBox.width - 1.f, nodeBox.top, 1.f, 1.f);
			boxBottomLeft.set(nodeBox.left, nodeBox.top + nodeBox.height - 1.f, 1.f, 1.f);
			boxBottomRight.set(nodeBox.left + nodeBox.width - 1.f, nodeBox.top + nodeBox.height -1.f, 1.f, 1.f);

			// If base/playertank/shell is further away than the node's centre then it updates the node to unknown
			if ((canSee(boxTopLeft) && canSee(boxTopRight)) ||
				(canSee(boxTopLeft) && canSee(boxBottomLeft)) ||
				(canSee(boxTopLeft) && canSee(boxBottomRight)) ||
				(canSee(boxTopRight) && canSee(boxBottomLeft)) ||
				(canSee(boxTopRight) && canSee(boxBottomRight)) ||
				(canSee(boxBottomLeft) && canSee(boxBottomRight)))
			{
				map.update(i, j);
			}*/
		}
	}

	for (int i = 0; i < s_kiNumRectShellPath; i++)
		shellPath[i].setFillColor(sf::Color(0, 0, 0, 0)); // Reset the fill colour of the shell path rectangles

	// Reset information about closest enemy
	iClosestEnemyObject = Object::UNKNOWN;
	closestEnemyPos = noEnemySeenPos;

	// Reset dodging flag
	bDodging = false;
	
	// If recently reset
	if (bResetFlag)
	{
		// If mode than 2 frames have passed
		if (iResetFrames > 2)
		{
			// Make a new path to the goal (Will go around bases now)
			map.makeNewPath(pos.getX(), pos.getY(), goalNode);
			bResetFlag = false; // To not do it again
			
		}	
		iResetFrames++; // Count frames passing
	}
}

void NewTank::checkStuck()
{
	// If position hasn't changed and it is meant to be moving forward or backward
	if ((pos.getX() == oldPos.getX()) && (pos.getY() == oldPos.getY()) && (!right) && (!left) && (forward || backward))
	{
		// Increment counter for amount of frames stuck
		iStuckFrames++;
	}
	else
	{
		// If no longer stuck, reset to 0
		iStuckFrames = 0;
	}

	// Changes state to stuck when stuck for 100 frames
	if (iStuckFrames >= kiStuckMinFrameCount)
	{
		iMovementState = AIMovementStates::STUCK;
		iStuckFrames = 0;
		//std::cout << "AI tank is stuck! State being reset." << std::endl;
	}
}

void NewTank::reset()
{
	bResetFlag = true;
	iResetFrames = 0;

	// Goes back to searching when respawning
	iMovementState = AIMovementStates::SEARCHING;

	//setStateMachineConditions();
	//moveStateMachineActions();
	//map.makeNewPath(pos.getX(), pos.getY(), goalNode);
	

	//sf::Vector2i nodePos = calcNodePos(pos.getX(), pos.getY()); // Get the map coordinates of the node spawned into

	//bool bCant = true; // For while loop
	//int iNum = 1; // Number for checking

	//iMovementState = AIMovementStates::RESETTING;

	/*map.setMapTraversable();

	while (bCant) // While true
	{
		// If the node spawned into is not traversable
		if (!map.traversable(map.getNodeObject(nodePos.x, nodePos.y)))
		{
			// If there is a node that is traversable iNum number of nodes to the left
			if (nodePos.x > iNum - 1 && map.traversable(map.getNodeObject(nodePos.x - iNum, nodePos.y)))
			{
				nodePos.x -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes to the right
			else if (nodePos.x < map.getWidth() - iNum && map.traversable(map.getNodeObject(nodePos.x + iNum, nodePos.y)))
			{
				nodePos.x += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes up
			else if (nodePos.y > iNum - 1 && map.traversable(map.getNodeObject(nodePos.x, nodePos.y - iNum)))
			{
				nodePos.y -= iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}
			// If there is a node that is traversable iNum number of nodes down
			else if (nodePos.y < map.getHeight() - iNum && map.traversable(map.getNodeObject(nodePos.x, nodePos.y + iNum)))
			{
				nodePos.y += iNum; // Set that node as the goal node
				bCant = false; // To leave the loop
			}

			sf::FloatRect nodeBorder = map.getNodeBox(nodePos.x, nodePos.y);
			sf::Vector2f nodeWorldPos = sf::Vector2f(nodeBorder.left + (nodeBorder.width / 2.f), nodeBorder.top + (nodeBorder.height / 2.f));
			resetTank(nodeWorldPos.x, nodeWorldPos.y, pos.getTh(), turretTh);
			//std::cout << "Changed pos" << std::endl;
		}
		else // If the goal node is traversable
			bCant = false; // To leave the loop
		iNum++; // Increase distance of node being checked

		std::cout << iNum << std::endl;
	}*/
}

void NewTank::collided()
{
	//right = true;
}

bool NewTank::isFiring()
{
	return bFiring; // Shoots if true
}

void NewTank::markTarget(Position p)
{
	//std::cout << "Enemy base spotted at (" << p.getX() << ", " << p.getY() << ")\n"; // Show in console
	// Get the global bounds of the thing spotted
	sf::FloatRect targetBounds = sf::FloatRect(sf::Vector2f(p.getX() - (kfBaseExtent/2), p.getY() - (kfBaseExtent / 2)), sf::Vector2f(kfBaseExtent, kfBaseExtent));
	// Tell map to mark it
	map.mark(targetBounds, Object::PLAYERBASE);
	// Compare the position with the current closest object
	comparePositions(sf::Vector2f(getX(), getY()), sf::Vector2f(p.getX(), p.getY()), Object::PLAYERBASE);
	//bCanSeeEnemyBase = true;
}

void NewTank::markEnemy(Position p)
{
	//std::cout << "Enemy tank spotted at (" << p.getX() << ", " << p.getY() << ")\n"; // Show in console
																					 // Get the global bounds of the thing spotted
	sf::FloatRect targetBounds = sf::FloatRect(sf::Vector2f(p.getX() - (kfTankExtent/2), p.getY() - (kfTankExtent/2)), sf::Vector2f(kfTankExtent, kfTankExtent));
	// Tell map to mark it
	map.mark(targetBounds, Object::PLAYERTANK);
	// Compare the position with the current closest object
	comparePositions(sf::Vector2f(getX(), getY()), sf::Vector2f(p.getX(), p.getY()), Object::PLAYERTANK);
	//bCanSeeEnemyTank = true;
}

void NewTank::markBase(Position p)
{
	//std::cout << "Friendly base spotted at (" << p.getX() << ", " << p.getY() << ")\n"; // Show in console
	// Get the global bounds of the thing spotted
	sf::FloatRect targetBounds = sf::FloatRect(sf::Vector2f(p.getX() - (kfBaseExtent / 2), p.getY() - (kfBaseExtent / 2)), sf::Vector2f(kfBaseExtent, kfBaseExtent));
	// Tell map to mark it
	map.mark(targetBounds, Object::OWNBASE);
}

void NewTank::markShell(Position p)
{
	//std::cout << "Shell spotted at (" << p.getX() << ", " << p.getY() << ")\n"; // Show in console
	// For the global bounds of every point in the path
	bDodging = true;

	// Bounding boxes for shell path
	sf::FloatRect targetBounds[s_kiNumRectShellPath];

	// Calculates difference in shell positions
	sf::Vector2f shellPosDiff = sf::Vector2f(p.getX(), p.getY()) - prevShellPos;
	prevShellPos = sf::Vector2f(p.getX(), p.getY());

	// Find angle
	float fAngle = calcAngle(shellPosDiff);

	for (int i = 0; i < s_kiNumRectShellPath; i++)
	{
		// Set the rectangle shapes so that they show the path the shell will take
		shellPath[i].setPosition(sf::Vector2f(p.getX() - (kfShellPathExtent/2), p.getY() - (kfShellPathExtent/2)) + (shellPosDiff * (kfShellPathDist * i)));
		// shellPath[i].setFillColor(sf::Color(255, 0, 0, 135));
		// Get the bounds of the point rectangle
		targetBounds[i] = sf::FloatRect(shellPath[i].getPosition(), shellPath[i].getSize());
		// Mark the point on the map
		map.mark(targetBounds[i], Object::PLAYERSHELL);
	}
	
	//map.mark(sf::FloatRect(shellPath.getPosition(), shellPath.getSize()), Object::SHELL);

	//std::cout << fAngle << std::endl;
}

void NewTank::score(int thisScore, int enemyScore)
{
	// Show the score in the console
	system("CLS");
	std::cout << "AI Score: " << thisScore << "\tPlayer Score: " << enemyScore << std::endl;
}

float NewTank::calcAngle(sf::Vector2f posDiff)
{
	// Top right quadrant
	if (posDiff.x >= 0 && posDiff.y < 0)
	{
		return RAD2DEG(asin(posDiff.x / sqrt((pow(posDiff.x, 2) + pow(posDiff.y, 2)))));
	}

	// Bottom right quadrant
	else if (posDiff.x >= 0 && posDiff.y >= 0)
	{
		return RAD2DEG(asin(posDiff.y / sqrt((pow(posDiff.x, 2) + pow(posDiff.y, 2))))) + 90;
	}

	// Bottom left quadrant
	else if (posDiff.x < 0 && posDiff.y < 0)
	{
		return RAD2DEG(acos(posDiff.y / sqrt((pow(posDiff.x, 2) + pow(posDiff.y, 2))))) + 180;
	}

	// Top left quadrant
	else if (posDiff.x < 0 && posDiff.y >= 0)
	{
		return RAD2DEG(acos(posDiff.x / sqrt((pow(posDiff.x, 2) + pow(posDiff.y, 2))))) + 90;
	}

	// Incase out of range
	else
	{
		return 0.f;
	}
}

void NewTank::comparePositions(sf::Vector2f tankPos, sf::Vector2f checkPos, Object type)
{
	// If the object being checked is closer than the current closest object
	if ((pow(tankPos.x - checkPos.x, 2) + pow(tankPos.y - checkPos.y, 2)) < (pow(tankPos.x - closestEnemyPos.x, 2) + pow(tankPos.y - closestEnemyPos.y, 2)))
	{
		// Replaces current closest object with checked object
		iClosestEnemyObject = Object::PLAYERBASE;
		closestEnemyPos = checkPos;
	}

	// Prioritises aiming at player's tank over player's bases
	if (type == Object::PLAYERTANK)
	{
		// Replaces current closest object with checked object
		iClosestEnemyObject = Object::PLAYERTANK;
		closestEnemyPos = checkPos;
	}
}

sf::Vector2i NewTank::calcNodePos(float fWorldX, float fWorldY)
{
	// For each node in the map
	for (int i = 0; i < map.getWidth(); i++)
	{ 
		for (int j = 0; j < map.getHeight(); j++)
		{
			// If the node contains the given world coordinates
			if (map.getNodeBox(i, j).intersects(sf::FloatRect(sf::Vector2f(fWorldX, fWorldY), sf::Vector2f(bb.getX2() - bb.getX1() + 1, bb.getY2() - bb.getY1() + 1))))
			{
				// Return the x and y values of the node
				return sf::Vector2i(i, j);
			}
		}
	}

	// If no node for given world space coordinates, gives top left node (Should never happen anyway)
	return sf::Vector2i(0, 0);
}

void NewTank::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	// Stuff to draw the tank
	target.draw(body);
	target.draw(turret);

	// Drawing stuff for debug mode
	if (debugMode)
	{
		target.draw(bb); // Drawing bounding box
		target.draw(map); // Draw the map nodes if in debug mode
		for (int i = 0; i < s_kiNumRectShellPath; i++)
			target.draw(shellPath[i]); // Draw the shell path if in debug mode
	}
}