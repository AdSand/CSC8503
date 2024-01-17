#include "PlayerObject.h"

void PlayerObject::ResetPlayer()
{
	GetPhysicsObject()->SetLinearVelocity({ 0, 0, 0 });
	GetPhysicsObject()->SetAngularVelocity({ 0, 0, 0 });
	GetTransform().SetPosition(playerStartPos);
}
