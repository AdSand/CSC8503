#include "C:/Users/b9022758/Documents/CSC8503Coursework/CSC8503 2023/CSC8503CoreClasses/CMakeFiles/CSC8503CoreClasses.dir/Debug/cmake_pch.hxx"
#include "HingeConstraint.h"
//#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "PhysicsObject.h"
//#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

HingeConstraint::HingeConstraint(GameObject* a, GameObject* b)
{
	objectA = a;
	objectB = b;
}

HingeConstraint::~HingeConstraint()
{

}

void HingeConstraint::UpdateConstraint(float dt) {

	Vector3 direction = (objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition()).Normalised();
	float newAngle = RadiansToDegrees(atan2f(-direction.z, direction.x));

	objectA->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, newAngle, 0));
	objectB->GetTransform().SetOrientation(objectA->GetTransform().GetOrientation());

	Vector3 temp = objectA->GetTransform().GetPosition();
	temp.y = objectB->GetTransform().GetPosition().y;
	objectA->GetTransform().SetPosition(temp);
}