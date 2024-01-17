#include "EnemyStateObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

EnemyStateObject::EnemyStateObject(GameObject* playerObject, int objectTag) {
	stateMachine = new StateMachine();
	tag = objectTag;
	player = playerObject;

	State* stateA = new State(
		[&](float dt)->void
		{
			this->Patrolling(dt);
		}
	);

	State* stateB = new State(
		[&](float dt)->void
		{
			this->AttackPlayer(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB,
		[&]()->bool
		{
			Vector3 dir = (player->GetTransform().GetPosition() - GetTransform().GetPosition());
			return dir.Length() < 20;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA,
		[&]()->bool
		{
			Vector3 dir = (player->GetTransform().GetPosition() - GetTransform().GetPosition());
			return dir.Length() > 25;;
		}
	));
}

EnemyStateObject::~EnemyStateObject() {
	delete stateMachine;
}

void EnemyStateObject::Update(float dt) {
	randomPos = Vector3(RandomValue(-20, 20), 0, RandomValue(-20, 20));
	stateMachine->Update(dt);
}

void EnemyStateObject::Patrolling(float dt) {
	GetPhysicsObject()->AddForce(randomPos);
}

void EnemyStateObject::AttackPlayer(float dt) {
	Vector3 dir = player->GetTransform().GetPosition() - GetTransform().GetPosition();
	dir.Normalise();
	GetPhysicsObject()->AddForce(dir * 3);
}