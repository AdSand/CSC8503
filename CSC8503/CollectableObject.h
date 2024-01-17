#pragma once
#include "TutorialGame.h"
#include <iostream>

using namespace NCL;
using namespace CSC8503;

#include "PhysicsObject.h"
#include "RenderObject.h"

class CollectableObject : public GameObject
{
	using GameObject::GameObject;

public:
	void OnCollisionBegin(GameObject* otherObject) override {
		if (otherObject->GetTag() == 1)
		{
			g->RemoveGameObject(this);

			Matrix4 view = g->GetMainCamera().BuildViewMatrix();
			Matrix4 camWorld = view.Inverse();

			Vector3 rightAxis = Vector3(camWorld.GetColumn(0));

			Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);

			fwdAxis.y = 0.0f;
			fwdAxis.Normalise();
			otherObject->GetPhysicsObject()->SetLinearVelocity({ 0, 0, 0 });
			otherObject->GetPhysicsObject()->ApplyLinearImpulse(fwdAxis * 30);
		}
	}

	void SetCurrentGameWorld(GameWorld* world) { g = world; }

protected:
	GameWorld* g;
};

