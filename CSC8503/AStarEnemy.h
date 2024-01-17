#pragma once
#include "TutorialGame.h"
#include <iostream>

using namespace NCL;
using namespace CSC8503;

#include "PhysicsObject.h"
#include "NavigationGrid.h"
#include "NavigationMesh.h"
#include <BehaviourSequence.h>
#include <BehaviourSelector.h>
#include <BehaviourAction.h>

class PlayerObject;

class AStarEnemy : public GameObject
{
	using GameObject::GameObject;

public:
	void OnCollisionBegin(GameObject* otherObject) override {
		if (otherObject->GetTag() != 1)
		{
			world->RemoveGameObject(otherObject);
		}
	}

	void SetPlayer(PlayerObject* player) { currentPlayer = player; }

	void SetGameWorld(GameWorld* gameworld) { world = gameworld; }

	void UpdateTree(float dt);

	void StartBehaviourTree();

protected:
	PlayerObject* currentPlayer;
	Vector3 playerPreviousPos = Vector3(0, 0, 0);

	vector<Vector3> nodes;
	int currentTargetNode;
	int currentNode;

	void CreatePath();
	void DisplayPath();
	void FollowPath();

	Vector3 RoundVectorToGrid(Vector3 playerPosition);

	GameWorld* world;

	BehaviourSequence* chasePlayerSequence = new BehaviourSequence("Chase Sequence");

	BehaviourSelector* waitPlayerSelection = new BehaviourSelector("Watch Selection");

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");

	void UpdatePath();

	BehaviourState state;

	float enemySpeed = 5;
};
