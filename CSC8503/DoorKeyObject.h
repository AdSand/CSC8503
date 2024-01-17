#pragma once
#include "TutorialGame.h"
#include <iostream>

using namespace NCL;
using namespace CSC8503;

#include "PhysicsObject.h"
#include "RenderObject.h"

class DoorKeyObject : public GameObject
{
	using GameObject::GameObject;

public:
	void OnCollisionBegin(GameObject* otherObject) override {
		if (otherObject->GetTag() == 1)
		{
			linkedDoor->GetTransform().SetPosition(linkedDoor->GetTransform().GetPosition() + Vector3(0, 8, 0));
			linkedDoor->GetRenderObject()->SetColour(Debug::GREEN);
			g->RemoveGameObject(this);
		}
	}

	void SetLinkedDoor(GameObject* door) { linkedDoor = door; }

	void SetCurrentGameWorld(GameWorld* world) { g = world; }


protected:
	GameObject* linkedDoor;
	GameWorld* g;
};

