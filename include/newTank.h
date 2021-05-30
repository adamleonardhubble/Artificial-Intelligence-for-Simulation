/*! \file newTank.h
* \brief Header file for the tank's AI and state machine (The NewTank class).
*
* Contains information about the AI, the states that the AI is currently in, and methods relating to the actions the AI tank will take.
*/

#pragma once

#include <iostream>
#include <list>
#include <stdlib.h> // Used for rand() function.

#include "aitank.h"
#include "map.h"
#include "playerTank.h"

/*! \class NewTank
* \brief Creates a tank with AI.
*
* The tank will move around the map and fire upon enemy objects depending on its state and what conditions are met.
*/
class NewTank : public AITank
{
private:
	Map map; //!< Stored map of the world.

	static const int s_kiNumRectShellPath = 4; //!< Number of points in the shell's path.

	sf::RectangleShape shellPath[s_kiNumRectShellPath]; //!< Used to know the path of the player shell.
	sf::Vector2f prevShellPos; //!< Stored position for the shell last frame.
	sf::Vector2f closestEnemyPos; //!< Position of the closest enemy object (With enemy tank being a priority).
	sf::Vector2f closestEnemyPosDiff; //!< Difference between position of tank and closest enemy object (With enemy tank being a priority).
	sf::Vector2f noEnemySeenPos = sf::Vector2f(-100.f, -100.f); //!< Used to reset closestEnemyPos when an enemy is not seen, so the tank does not think a destroyed enemy object is still there.
	sf::Vector2f currentTankPos; //!< Current position of tank in world space
	sf::Vector2f prevTankPos; //!< Previous position of tank in world space

	float fAngleDiff; //!< The clockwise angle between the AI tank and the closest enemy base or player in degrees, where directly right is 0 degrees (With enemy tank being a priority).
	float fDistanceToTarget; //!< Scalar distance to closest enemy base or player (With enemy tank being a priority).

	bool bFiring; //!< Should the tank be firing.
	bool bDodging; //!< Should the tank be dodging.
	bool bCanSeeEnemyTank; //!< If it can see an enemy tank.
	bool bCanSeeEnemyBase; //!< If it can see an enemy base.
	bool bResetFlag; //!< Flag for resetting.

	sf::Vector2i goalNode; //!< Node position of the goal (X node, Y node).
	sf::Vector2i prevGoalNode; //!< Node position of the goal last frame (X node, Y node).
	sf::Vector2i currentNode; //!< Node position the AI tank is currently in (X node, Y node).
	sf::Vector2i prevNode; //!< Node position the AI tank was in last frame (X node, Y node).

	enum AIMovementStates { DODGING = 0, ESCAPING = 1, HIDING = 2, STOPPING = 3, FOLLOWING = 4, SEARCHING = 5, STUCK = 6 }; //!< Enum used for the state machine for the AI tank's movement.
	enum AIWeaponStates { SEARCHINGAIM = 0, AIMING = 1 }; //!< Enum used for the state machine for the AI tank's turret.
	
	int iClosestEnemyObject; //!< Object type of closest enemy object.
	int iMovementState; //!< Current state the AI tank is in.
	int iWeaponState; //!< Current state the AI tank's turret is in.
	int iPrevMovementState; //!< State the AI tank was in last frame.
	int iFollowingFrameCount = 0; //!< To prevent rapid change between FOLLOWING and STOPPING states.
	int iStuckFrames = 0; //!< Amount of frames AI tank has been stuck.
	int iLeftFrames = 0; //!< Amount of frames tank has been turning left.
	int iRightFrames = 0; //!< Amount of frames tank has been turning right.
	int iLostPlayerTankFrames = 0; //!< Amount of frames AI tank has not seen the player tank.

	const int kiFollowingMinFrameCount = 20; //!< To prevent rapid change between FOLLOWING and STOPPING states (Limit).
	const int kiStuckMinFrameCount = 100; //!< How many frames minimum the tank should be stuck before transitioning into the STUCK state.
	const int kiTurningMinFrameCount = 20; //!< How many frames minimum the tank should be turning before turret searches in a different direction.
	const int kiLostPlayerMinFrameCount = 20; //!< How many frames minimum the player tank should not be visible before transitioning to the FOLLOWING state.

	const float kfShellPathExtent = 75.f; //!< The width/height of the square bounding box for each part of the shell path.
	const float kfShellPathDist = 25.f; //!< How far each bounding box of the shell path should be from each other.
	const float kfViewDistance = 250.f; //!< How far the tank should be from an enemy object before shooting, view distance of turret.
	const float kfTurretAccuracyLimit = 1.25f; //!< How close the turret should be (in degrees) before shooting at the target.
	const float kfBodyAccuracyLimit = 2.f; //!< How close the tank's body should be (in degrees) before no longer rotating.
	const float kfBaseExtent = 20.f; //!< The width/height of a base.
	const float kfTankExtent = 40.f; //!< The width/height of a tank.
	

	int iResetFrames; //!< Amount of frames since reset.

	/* AIMovementStates:

	STUCK: State where tank has been stuck for a certain amount of frames. Used to reset state, allowing a new path to be calculated.
		Entry: Tank not moved for X frames when supposed to be moving.
		Process: N/A
		Exit: Another condition for a state is met.

	DODGING: Used to dodge out of the way of enemy shells.
		Entry: An enemy shell has entered the AI tank's view AND is predicted to hit the AI tank.
		Process: ?
		Exit: Higher priority state meets conditions, or entry conditions are no longer met.

	ESCAPING: Used to run away from the player if out of ammo.
		Entry: No ammo AND enemy tank is in view.
		Process: A* algorithm calculates path away from the player, tank rotates and moves towards goal.
		Exit: Higher priority state meets conditions, or entry conditions are no longer met.

	HIDING: Used to hide from the player if out of ammo (Default state once out of ammo).
		Entry: No ammo.
		Process: A* algorithm calculates the closest path to a corner of the map, AI tank follows this path.
		Exit: Higher priority state meets conditions, or entry conditions are no longer met.

	STOPPING: Used to stop and aim at a target.
		Entry: Enemy object is in view.
		Process: A* algorithm calculates the closest path to the enemy object to be stored for the FOLLOWING state, AI tank stops and sets the turret's state to AIMING.
		Exit: Higher priority state meets conditions, or entry conditions are no longer met.

	FOLLOWING:
		Entry: An enemy object that was previously in view has left its view.
		Process: Moves to last known location of the enemy object (Path calculated when in STOPPED state).
		Exit: Higher priority state meets conditions, entry conditions are no longer met or goal node has been reached.

	SEARCHING: Default state where the tank moves in a straight line towards a point on the map (Not sure if this will be random) using A* search.
		Entry: Tank spawned.
		Process: A* algorithm calculates a path towards somewhere random on the map and the AI tank follows this path.
		Exit: Higher priority state meets conditions, entry conditions are no longer met or goal node has been reached.

	Priorities:
		1 - STUCK
		2 - DODGING
		3 - ESCAPING
		4 - HIDING
		5 - STOPPING
		6 - FOLLOWING
		7 - SEARCHING
	*/

	/*****************************************************************************************************************************************************************/

	/* AIWeaponStates:

	AIMING: Used to aim at an enemy object.
		Entry: STOPPING or ESCAPING.
		Process: Aims turret towards viewed enemy object.
		Exit: No enemy objects in view.

	SEARCHINGAIM: Default state where the turret spins 360 degrees constantly in an attempt to view an enemy object
		Entry: Tank spawned, SEARCHING, DODGING, HIDING or FOLLOWING.
		Process: Turret rotates clockwise.
		Exit: N/A.

	Priorities:
		1 - AIMING
		2 - SEARCHINGAIM
	*/

public:
	NewTank(); //!< Default constructor for NewTank.
	void setTargetInfo(); //!< Sets information about the target (Vector distance to target, scalar distance to target, angle difference between AI tank and target and AI tank's current node).
	void checkVision(); //!< Checks if the AI tank can see any enemy objects.
	void setStateMachineConditions(); //!< Sets states of AI tank depending on what conditions are met.
	void moveStateMachineActions(); //!< AI tank performs actions depending on its state.
	void turretStateMachineActions(); //!< Turret performs actions depending on its state.
	void aimTurret(); //!< Rotates turret to point at target.
	void move(); //!< Tank moves along calculated path.
	void checkStuck(); //!< Checks if the tank is stuck and not moving when it should be. If so, a new goal node is calculated next frame.
	void reset(); //!< Resets tank, making sure it doesn't spawn inside a non-traversable object.
	void collided(); //!< Tank has collided with a bounding box.
	bool isFiring(); //!< Checks if tank should be firing.

	//! Marks enemy bases on the map that are in the AI tank's vision.
	/*!
	* \param p Position of the base.
	*/
	void markTarget(Position p); 

	//! Marks enemy tanks on the map that are in the AI tank's vision.
	/*!
	* \param p Position of the enemy tank.
	*/
	void markEnemy(Position p);

	//! Marks friendly bases on the map that are in the AI tank's vision.
	/*!
	* \param p Position of the base.
	*/
	void markBase(Position p);

	//! Marks enemy shells on the map that are in the AI tank's vision.
	/*!
	* \param p Position of the shell.
	*/
	void markShell(Position p);

	//! Prints the player and AI tank's score to the console.
	/*!
	* \param thisScore The player's score.
	* \param enemyScore The AI tank's score.
	*/
	void score(int thisScore, int enemyScore);

	//! Calculates an angle between the AI tank and the target using the difference in their positions.
	/*!
	* \param posDiff The vector distance between the AI tank and the target.
	*/
	float calcAngle(sf::Vector2f posDiff);

	//! Compares the position of the current object being checked, with the current object that is closest to the AI tank to see which is closer (Makes enemy tank a priority even if a base is closer).
	/*!
	* \param tankPos The vector position of the tank.
	* \param checkPos The vector position of the object being checked.
	* \param type The object being checked's type.
	*/
	void comparePositions(sf::Vector2f tankPos, sf::Vector2f checkPos, Object type);

	//! Returns the node position of a position in the world.
	/*!
	* \param fWorldX The X position in the world.
	* \param fWorldY The Y position in the world.
	*/
	sf::Vector2i calcNodePos(float fWorldX, float fWorldY);

	//! Overrider for the tank draw function.
	/*!
	* \param target The target being rendered to.
	* \states Render states.
	*/
	void draw(sf::RenderTarget &target, sf::RenderStates states) const; 
};