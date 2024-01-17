#include "AStarEnemy.h"

void AStarEnemy::UpdatePath()
{
	if (RoundVectorToGrid(currentPlayer->GetTransform().GetPosition()) != playerPreviousPos)
	{
		nodes.clear();
		CreatePath();
		playerPreviousPos = RoundVectorToGrid(currentPlayer->GetTransform().GetPosition());
		currentNode = 0;
	}
	FollowPath();
	DisplayPath();
}

void AStarEnemy::UpdateTree(float dt)
{
	// run the tree
	state = rootSequence->Execute(dt); // fake dt
	if (state == Success)
	{
		rootSequence->Reset();
		//std::cout << "I got the player!\n";
	}
	else if (state == Failure)
	{
		rootSequence->Reset();
		//std::cout << "Player is still playing!\n";
	}
}

void AStarEnemy::StartBehaviourTree()
{
	BehaviourAction* checkPlayerHasItem = new BehaviourAction("Check for item",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				//std::cout << "Checking the player for item\n";
				state = Ongoing;
			}
			else if (state == Ongoing)
			{
				if (currentPlayer->GetHasHeistItem())
				{
					//std::cout << "Player has the item!\n";
					return Success;
				}
				else
				{
					// player doesn't have the item
					//std::cout << "Player is empty!\n";
					return Failure;
				}
			}
			return state; // will be ongoing until success
		}
	);

	BehaviourAction* updatePathFinding = new BehaviourAction("Follow the player",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				//std::cout << "Updating the path finding\n";
				state = Ongoing;
			}
			else if (state == Ongoing)
			{
				if (GetTransform().GetPosition() == currentPlayer->GetTransform().GetPosition())
				{
					//std::cout << "we have hit the player. Success!\n";
					return Success;
				}
				UpdatePath();
			}
			return state; // will be ongoing until success
		}
	);

	BehaviourAction* lookAtPlayer = new BehaviourAction("looking at the player",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				//std::cout << "Watching the player\n";
				Vector3 delta = currentPlayer->GetTransform().GetPosition() - GetTransform().GetPosition();
				float direction = atan2f(-delta.x, -delta.z);
				Quaternion rotation = Quaternion::EulerAnglesToQuaternion(0, RadiansToDegrees(direction), 0).Normalised();
				GetTransform().SetOrientation(rotation);

				return Success;
			}
			return state;
		}
	);

	BehaviourAction* changeColours = new BehaviourAction("Change colour",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				//std::cout << "Changing to a random colour\n";
				return Ongoing;
			}
			else if (state == Ongoing)
			{
				bool change = rand() % 2;
				if (change)
				{
					//std::cout << "I have changed colour!\n";
					Vector4 newColour;
					newColour.x = rand() / double(RAND_MAX);
					newColour.y = rand() / double(RAND_MAX);
					newColour.z = rand() / double(RAND_MAX);
					newColour.w = 1;
					GetRenderObject()->SetColour(newColour);
					return Success;
				}
				//std::cout << "Decided not to change!\n";
				return Failure;
			}
			return state;
		}
	);

	chasePlayerSequence->AddChild(checkPlayerHasItem);
	chasePlayerSequence->AddChild(updatePathFinding);

	waitPlayerSelection->AddChild(changeColours);
	waitPlayerSelection->AddChild(lookAtPlayer);

	rootSequence->AddChild(waitPlayerSelection);
	rootSequence->AddChild(chasePlayerSequence);
}

void AStarEnemy::CreatePath()
{
	NavigationGrid grid("WorldGrid.txt");

	NavigationPath outPath;

	Vector3 startPos(RoundVectorToGrid(GetTransform().GetPosition()));
	Vector3 endPos(RoundVectorToGrid(currentPlayer->GetTransform().GetPosition()));

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos))
	{
		nodes.push_back(pos);
	}
}

void AStarEnemy::DisplayPath()
{
	for (int i = 1; i < nodes.size(); i++)
	{
		Vector3 a = nodes[i - 1];
		Vector3 b = nodes[i];

		Debug::DrawLine(a, b, Debug::RED);
	}
}

void AStarEnemy::FollowPath()
{
	if (nodes.size() == 0) { return; } // no path to follow

	if ((RoundVectorToGrid(currentPlayer->GetTransform().GetPosition())) != (RoundVectorToGrid(GetTransform().GetPosition())))
	{
		Vector3 direction = nodes[currentNode + 1] - GetTransform().GetPosition();
		direction.y = 0;

		if (direction.Length() < 0.5f)
		{
			currentNode++;
		}
		else
		{
			GetPhysicsObject()->SetLinearVelocity(direction.Normalised() * enemySpeed);
		}
	}
}

Vector3 AStarEnemy::RoundVectorToGrid(Vector3 position)
{
	Vector3 tempVec;
	tempVec.x = round(position.x / 20.0f) * 20.0f;
	tempVec.y = 0;
	tempVec.z = round(position.z / 20.0f) * 20.0f;
	return tempVec;
}


