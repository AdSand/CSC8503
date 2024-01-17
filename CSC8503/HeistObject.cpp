#include "HeistObject.h"

bool HeistObject::CheckHeistState()
{
	Vector3 dir = heistDestination - GetTransform().GetPosition();
	dir.y = 0;
	return dir.Length() < 10.0f;
}
