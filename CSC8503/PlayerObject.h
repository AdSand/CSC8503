#pragma once
#include "TutorialGame.h"
#include <iostream>

#include <string>

using namespace NCL;
using namespace CSC8503;

#include "PhysicsObject.h"
#include "RenderObject.h"

class PlayerObject : public GameObject
{
	using GameObject::GameObject;

public:
	void OnCollisionBegin(GameObject* otherObject) override {
		switch (otherObject->GetTag())
		{
		case 2:
			// Hit a key
			AddToScore(35);
			break;
		case 3:
			// hit a collectable
			collectableCount++;
			AddToScore(20);
			break;
		case 4:
			// Hit heist item
			hasHeistItem = true;
			AddToScore(500);
			break;
		case 5:
			// hit a state machine enemy
			TakeDamage(1);
			if (!hasHeistItem) ResetPlayer();
			break;
		case 6:
			// hit the A star enemy
			TakeDamage(5);
			break;
		case 7:
			// hit a moveable object that gives points
			AddToScore(1);
			break;
		}
	}

	void AddToScore(float value) { score += value; }
	int GetScore() { return score; }

	int GetLives() { return lives; }

	int GetCollectableCount() { return collectableCount; }

	bool GetHasHeistItem() { return hasHeistItem; }

	void TakeDamage(int damage) { lives -= damage; }

	Vector3 GetStartPos() { return playerStartPos;  }

	void ResetPlayer();

protected:
	int score = 100;
	int collectableCount = 0;
	bool hasHeistItem = false;
	int lives = 5;
	Vector3 playerStartPos = Vector3(60, 4, 0);
};
