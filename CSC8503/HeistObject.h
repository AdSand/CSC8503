#pragma once
#include "TutorialGame.h"
#include <iostream>

using namespace NCL;
using namespace CSC8503;

#include "PhysicsObject.h"

class HeistObject : public GameObject
{
	using GameObject::GameObject;

public:
	void OnCollisionBegin(GameObject* otherObject) override {
		if (otherObject->GetTag() == 1)
		{
			isCollected = true;
		}
	}

	void SetPlayer(GameObject* player) { currentPlayer = player; }

	void SetHeistDestination() { heistDestination = currentPlayer->GetTransform().GetPosition(); }

	bool IsItemCollected() { return isCollected; }

	bool CheckHeistState();


protected:
	GameObject* currentPlayer;
	Vector3 heistDestination;
	bool isCollected = false;
};

