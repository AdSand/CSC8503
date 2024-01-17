#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"

#include <iostream>
#include <fstream>




using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame() : controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()) {
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
	renderer->Init();
	renderer->InitStructures();
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= true;
	physics->UseGravity(useGravity);
	inSelectionMode = false;


	world->GetMainCamera().SetController(controller);

	controller.MapAxis(0, "Sidestep");
	controller.MapAxis(1, "UpDown");
	controller.MapAxis(2, "Forward");

	controller.MapAxis(3, "XLook");
	controller.MapAxis(4, "YLook");
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	charMesh	= renderer->LoadMesh("goat.msh");
	enemyMesh	= renderer->LoadMesh("Keeper.msh");
	//bonusMesh	= renderer->LoadMesh("apple.msh");
	bonusMesh	= renderer->LoadMesh("cube.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");
	gooseMesh = renderer->LoadMesh("goose.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");

	InitCamera();
	//InitWorld();
	ResetGame();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;
	delete gooseMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete server;
	delete client;

	delete player;

	delete enemy1;
	delete enemy2;
	delete AStarChaseEnemy;

	delete heistObject;
}

void TutorialGame::UpdateGame(float dt) {	
	if (!inSelectionMode) {
		world->GetMainCamera().UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		Vector3 objPos = player->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera().SetPosition(camPos);
		world->GetMainCamera().SetPitch(angles.x);
		world->GetMainCamera().SetYaw(player->GetTransform().GetOrientation().ToEuler().y);
	}

	UpdateKeys();

	RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world->Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		}
	}

	SelectObject();
	MoveSelectedObject();

	if (enemy1) enemy1->Update(dt);
	if (enemy2) enemy2->Update(dt);
	AStarChaseEnemy->UpdateTree(dt);

	if (heistObject->CheckHeistState())
	{
		// player has won the game
		player->AddToScore(500);
		if (isClient)
		{
			HighScorePacket sentScore = player->GetScore();
			client->SendPacket(sentScore);
		}
		isGameOver = true;
	}

	// player has grabbed the item
	if (heistObject->IsItemCollected())
	{
		// set object above the player
		heistObject->GetTransform().SetPosition(player->GetTransform().GetPosition() + Vector3(0, 3, 0));
		Debug::Print("RUN!!!!", Vector2(2, 5), Debug::RED);
	}
	else
	{
		Debug::Print("Collectables Remaining: " + std::to_string(
			collectablesInWorld - player->GetCollectableCount()), Vector2(2, 5), Debug::WHITE);
	}

	// update game info
	timer -= dt;
	Debug::Print("Timer: " + std::to_string((int)timer), Vector2(2, 10), Debug::WHITE);
	Debug::Print("Score: " + std::to_string(player->GetScore()), Vector2(2, 15), Debug::WHITE);
	Debug::Print("Lives: " + std::to_string(player->GetLives()), Vector2(2, 20), Debug::WHITE);
	Debug::Print("Press H for hook", Vector2(2, 25), Debug::WHITE);
	
	// Scenarios to end the game
	if (timer <= 0.0f || player->GetLives() <= 0 || player->GetTransform().GetPosition().y <= -10)
	{
		if (isClient)
		{
			HighScorePacket sentScore = player->GetScore();
			client->SendPacket(sentScore);
		}
		isGameOver = true;
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);
	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void NCL::CSC8503::TutorialGame::UpdateMenu(float dt)
{
	Debug::Print("Press Space for single player", Vector2(30, 50), Debug::BLUE);
	Debug::Print("Press C for client", Vector2(30, 60), Debug::BLUE);
	Debug::Print("Press S for server", Vector2(30, 70), Debug::BLUE);
	Debug::Print("Press escape to exit", Vector2(30, 80), Debug::BLUE);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void NCL::CSC8503::TutorialGame::UpdatePause(float dt)
{
	Debug::Print("Game Paused", Vector2(30, 50), Debug::BLUE);
	Debug::Print("Press U to unpause", Vector2(30, 60), Debug::BLUE);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);
	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void NCL::CSC8503::TutorialGame::UpdateEndScreen(float dt)
{
	Debug::Print("Final Score: " + std::to_string(player->GetScore()), Vector2(30, 50), Debug::BLUE);
	Debug::Print("Press U to play again", Vector2(30, 60), Debug::BLUE);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);
	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void NCL::CSC8503::TutorialGame::ResetGame()
{
	isGameOver = false;
	serverHighScore = NULL;
	InitWorld();
}

void NCL::CSC8503::TutorialGame::InitialiseAsSinglePlayer()
{
	InitialiseAssets();
}

void NCL::CSC8503::TutorialGame::InitialiseAsServer()
{
	NetworkBase::Initialise();
	int port = NetworkBase::GetDefaultPort();
	isClient = false;

	// clear the high score file as it's a new server
	std::ofstream file(Assets::DATADIR + "HighScores.txt");
	file.clear();
	file.close();

	server = new GameServer(port, 4);
	server->RegisterPacketHandler(String_Message, this);
	server->RegisterPacketHandler(Int_Message, this);
	server->RegisterPacketHandler(HighScore_Message, this);
}

void NCL::CSC8503::TutorialGame::UpdateAsServer(float dt)
{
	Debug::Print("This is a server", Vector2(10, 10), Debug::GREEN);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	renderer->Render();
	Debug::UpdateRenderables(dt);
	server->UpdateServer();
}


void NCL::CSC8503::TutorialGame::InitialiseAsClient()
{
	NetworkBase::Initialise();
	int port = NetworkBase::GetDefaultPort();
	isClient = true;

	client = new GameClient();
	bool canConnect = client->Connect(127, 0, 0, 1, port);
	client->RegisterPacketHandler(String_Message, this);
	client->RegisterPacketHandler(HighScore_Message, this);

	InitialiseAssets();
}

void NCL::CSC8503::TutorialGame::UpdateAsClient(float dt)
{
	Debug::Print("This is a client", Vector2(2, 85), Debug::GREEN);

	if (serverHighScore != NULL)
	{
		Debug::Print("Server high score: " + std::to_string(serverHighScore), Vector2(2, 95), Debug::BLUE);
	}
	else
	{
		Debug::Print("Press Z to load high score", Vector2(2, 95), Debug::RED);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::V)) {
		StringPacket p2 = "Client says hello!" + std::to_string(2);
		client->SendPacket(p2);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::Z)) {
		IntPacket highscoreRequest = 0;
		client->SendPacket(highscoreRequest);
	}

	UpdateGame(dt);
	client->UpdateClient();
}

void NCL::CSC8503::TutorialGame::ReceivePacket(int type, GamePacket* payload, int source)
{
	if (isClient)
	{
		switch (type)
		{
		case String_Message:
		{
			StringPacket* realPacket = (StringPacket*)payload;
			std::string msg = realPacket->GetStringFromData();
			std::cout << "Client recieved message " << msg << std::endl;
			break;
		}
		case HighScore_Message:
		{
			// Server has sent the high score
			HighScorePacket* realPacket = (HighScorePacket*)payload;
			serverHighScore = realPacket->GetHighscoreFromData();
			std::cout << "Current high score on server: " << serverHighScore << std::endl;
			break;
		}
		}
	}
	else // is a server
	{
		switch (type)
		{

		case String_Message:
		{
			StringPacket* realPacket = (StringPacket*)payload;
			std::string msg = realPacket->GetStringFromData();
			std::cout << "server recieved message " << msg << std::endl;
			break;
		}

		case Int_Message:
		{
			IntPacket* realPacket = (IntPacket*)payload;
			int msg = realPacket->GetIntFromData();
			std::cout << "Server has recieved request for: " << msg << std::endl;

			// switch through different requests
			if (msg == 0)
			{
				// client requested a high score. Send to only that client.
				HighScorePacket highscoreRequest = GetHighScoreFromFile();
				server->SendPacketToSinglePeer(highscoreRequest, source);
			}
			break;
		}

		case HighScore_Message:
		{
			// Client has sent a score to be added to high score table
			HighScorePacket* realPacket = (HighScorePacket*)payload;
			int scoreToBeAdded = realPacket->GetHighscoreFromData();
			AddScoreToFile(scoreToBeAdded);
			std::cout << scoreToBeAdded << " added to the high score file." << std::endl;
			break;
		}
		}
	}
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::H)) {
		int grappleHookLength = 25;

		Vector3 inFront = player->GetTransform().GetPosition() + GetWorldForwardAxis();
		inFront.Normalised();

		Vector3 direction = inFront - player->GetTransform().GetPosition();
		direction.Normalised();

		Ray grappleHook(player->GetTransform().GetPosition(), direction);

		RayCollision grappleCollision;
		if (world->Raycast(grappleHook, grappleCollision, true, player))
		{
			GameObject* temp = (GameObject*)grappleCollision.node;
			if ((player->GetTransform().GetPosition() - temp->GetTransform().GetPosition()).Length() <= grappleHookLength)
			{
				Debug::DrawLine(player->GetTransform().GetPosition(), temp->GetTransform().GetPosition());
				Vector3 hookDirection = temp->GetTransform().GetPosition() - player->GetTransform().GetPosition();
				hookDirection.Normalised();
				player->GetPhysicsObject()->ApplyLinearImpulse(hookDirection * 0.2f);
			}
			else
			{
				Debug::DrawLine(player->GetTransform().GetPosition(), player->GetTransform().GetPosition() + direction * grappleHookLength);
			}
		}

	}

	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::F8)) {
		world->ShuffleObjects(false);
	}

	LockedObjectMovement();
}

void TutorialGame::LockedObjectMovement() {
	Vector3 fwdAxis = GetWorldForwardAxis();
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();
	float speed = 10;;

	if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
		player->GetPhysicsObject()->AddForce(fwdAxis * speed);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
		player->GetPhysicsObject()->AddForce(-fwdAxis * speed);
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
		player->GetPhysicsObject()->AddTorque(Vector3(0, 2.5f, 0));
	}

	if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
		player->GetPhysicsObject()->AddTorque(Vector3(0, -2.5f, 0));
	}

	// used for testing levels. Delete this later
	if (Window::GetKeyboard()->KeyDown(KeyCodes::SPACE)) {
		player->GetPhysicsObject()->AddForce(Vector3(0, 25, 0));
	}

	lockedOffset = Matrix4(player->GetTransform().GetOrientation().Normalised())
		* Matrix4::Translation(Vector3(0, 30, 50)).GetPositionVector();

}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera().SetNearPlane(0.1f);
	world->GetMainCamera().SetFarPlane(500.0f);
	world->GetMainCamera().SetPitch(-15.0f);
	world->GetMainCamera().SetYaw(300.0f);
	world->GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	timer = 300;

	// reset the chase enemy
	AStarChaseEnemy = nullptr;

	// setup the player
	player = AddPlayerToWorld();
	player->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 90, 0));
	AddCubeToWorld(player->GetTransform().GetPosition() - Vector3(0, 1, 0), Vector3(4, 0.01f, 4), 0)
		->GetRenderObject()->SetColour(Debug::GREEN);
	lockedObject = player;

	// setup the gates
	AddGateToWorld(Vector3(12, 6, 20));
	AddGateToWorld(Vector3(72, 6, 140));

	// setup the layout of the map
	BuildFromFile("FloorLayout.txt", 0);
	BuildFromFile("MazeLayout.txt", 6);

	// setup obstacles
	AddOBBCubeToWorld(Vector3(0, 6, 60), Vector3(10, 2, 20), 0)
		->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(25, 90, 0));
	AddBridgeToWorld(Vector3(201, 2, 140));
	AddBallPitToWorld();
	AddDoorsToWorld();

	// setup enemies
	enemy1 = AddEnemyStateObjectToWorld(Vector3(80, 5, 40));
	enemy1->GetRenderObject()->SetColour(Debug::RED);
	enemy2 = AddEnemyStateObjectToWorld(Vector3(100, 5, 40));
	enemy2->GetRenderObject()->SetColour(Debug::RED);

	AStarChaseEnemy = AddAStarEnemyToWorld({ 200, 5, 20 }, 8);
	AStarChaseEnemy->SetPlayer(player);
	AStarChaseEnemy->SetGameWorld(world);
	AStarChaseEnemy->StartBehaviourTree();

	// setup the heist
	heistObject = AddHeistObjectToWorld(Vector3(160, 5, 20));
	heistObject->SetPlayer(player);
	heistObject->SetHeistDestination();
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, Vector3 floorSize) {
	GameObject* floor = new GameObject();
	 
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject(7);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetRenderObject()->SetColour(Vector4(0.5f, 1, 0.5f, 1));

	world->AddGameObject(cube);

	return cube;
}

GameObject* NCL::CSC8503::TutorialGame::AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass)
{
	GameObject* cube = new GameObject();

	OBBVolume* volume = new OBBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* NCL::CSC8503::TutorialGame::AddCapsuleToWorld(const Vector3& position, float radius, float halfHeight, float inverseMass)
{
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2));

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;
}



PlayerObject* TutorialGame::AddPlayerToWorld() {
	float meshSize		= 1.0f;
	float inverseMass	= 0.5f;

	PlayerObject* character = new PlayerObject(1);
	SphereVolume* volume  = new SphereVolume(1.0f);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(character->GetStartPos());

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetRenderObject()->SetColour(Vector4(0, 0.6f, 0.95f, 1));

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetRenderObject()->SetColour(Vector4(1, 0.33f, 0, 1));

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

void NCL::CSC8503::TutorialGame::BuildFromFile(std::string filename, float height)
{
	std::ifstream file(Assets::DATADIR + filename);
	std::string line;
	if (!file.is_open()) std::cout << "Couldn't load the file\n";
	collectablesInWorld = 0;
	int currentRow = 0;
	while (std::getline(file, line))
	{
		int currentChar = 0;
		for (char& c : line)
		{
			Vector3 buildPosition = Vector3(currentChar * 20, height, currentRow * 20);
			switch (c)
			{
			case 'X':
				AddCubeToWorld(buildPosition, Vector3(10, 3, 10), 0);
				break;
			case 'C':
				CollectableObject* c;
				c = AddCollectableToWorld({ buildPosition.x, buildPosition.y - 2, buildPosition.z });
				c->SetCurrentGameWorld(world);
				collectablesInWorld++;
				break;
			case 'U':
				AddCapsuleToWorld(buildPosition, 1.5f, 3, 0)
					->GetRenderObject()->SetColour(Debug::GREEN);
				break;
			case 'O':
				break;
			}
			currentChar++;
		}
		currentRow++;
	}
	file.close();
}



void TutorialGame::AddBridgeToWorld(Vector3 startPos)
{
	Vector3 cubeSize = Vector3(10.5f, 1, 10);

	float invCubeMass = 5; // how heavy middle pieces are
	float numLinks = 8;
	float maxDistance = 21; // constraint distance
	float cubeDistance = 20; // distance between links


	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0); // infinite mass
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0); // infinite mass

	GameObject* previous = start;

	for (int i = 0; i < numLinks; i++)
	{
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void NCL::CSC8503::TutorialGame::AddGateToWorld(Vector3 startPos)
{
	Vector3 sideSizes = Vector3(1, 3, 1);
	Vector3 doorSizes = Vector3(3, 2, 1);

	GameObject* gateLeft = AddCubeToWorld(startPos, sideSizes, 0); // infinite mass
	GameObject* doorLeft = AddCubeToWorld(startPos + Vector3(6, 0, 0), doorSizes);
	HingeConstraint* gateConstraint1 = new HingeConstraint(gateLeft, doorLeft);
	PositionConstraint* positionConstraint1 = new PositionConstraint(gateLeft, doorLeft, 5);

	GameObject* gateRight = AddCubeToWorld(startPos + Vector3(16, 0, 0), sideSizes, 0); // infinite mass
	GameObject* doorRight = AddCubeToWorld(startPos + Vector3(10.5f, 0, 0), doorSizes);
	HingeConstraint* gateConstraint2 = new HingeConstraint(gateRight, doorRight);
	PositionConstraint* positionConstraint2 = new PositionConstraint(gateRight, doorRight, 5);

	gateLeft->GetRenderObject()->SetColour(Vector4(0.59f, 0.29f, 0, 1));
	doorLeft->GetRenderObject()->SetColour(Vector4(0.59f, 0.29f, 0, 1));
	gateRight->GetRenderObject()->SetColour(Vector4(0.59f, 0.29f, 0, 1));
	doorRight->GetRenderObject()->SetColour(Vector4(0.59f, 0.29f, 0, 1));

	world->AddConstraint(gateConstraint1);
	world->AddConstraint(gateConstraint2);
	world->AddConstraint(positionConstraint1);
	world->AddConstraint(positionConstraint2);
}

DoorKeyObject* NCL::CSC8503::TutorialGame::AddKeyToTheWorld(const Vector3& position, float radius)
{
	DoorKeyObject* sphere = new DoorKeyObject(2);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(0);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

HeistObject* NCL::CSC8503::TutorialGame::AddHeistObjectToWorld(const Vector3& position)
{
	HeistObject* goldenGoat = new HeistObject(4);
	SphereVolume* volume = new SphereVolume(1.0f);

	goldenGoat->SetBoundingVolume((CollisionVolume*)volume);

	goldenGoat->GetTransform()
		.SetScale(Vector3(1, 1, 1))
		.SetPosition(position);

	goldenGoat->SetRenderObject(new RenderObject(&goldenGoat->GetTransform(), charMesh, nullptr, basicShader));
	goldenGoat->SetPhysicsObject(new PhysicsObject(&goldenGoat->GetTransform(), goldenGoat->GetBoundingVolume()));

	goldenGoat->GetPhysicsObject()->SetInverseMass(0);
	goldenGoat->GetPhysicsObject()->InitSphereInertia();
	goldenGoat->GetRenderObject()->SetColour(Vector4(1, 0.9f, 0, 1));

	world->AddGameObject(goldenGoat);

	return goldenGoat;
}

AStarEnemy* NCL::CSC8503::TutorialGame::AddAStarEnemyToWorld(const Vector3& position, float radius)
{
	AStarEnemy* aStarEnemy = new AStarEnemy(6);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	aStarEnemy->SetBoundingVolume((CollisionVolume*)volume);

	aStarEnemy->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	aStarEnemy->SetRenderObject(new RenderObject(&aStarEnemy->GetTransform(), gooseMesh, basicTex, basicShader));
	aStarEnemy->SetPhysicsObject(new PhysicsObject(&aStarEnemy->GetTransform(), aStarEnemy->GetBoundingVolume()));

	aStarEnemy->GetPhysicsObject()->SetInverseMass(0);
	aStarEnemy->GetPhysicsObject()->InitSphereInertia();
	aStarEnemy->GetRenderObject()->SetColour(Debug::RED);

	world->AddGameObject(aStarEnemy);

	return aStarEnemy;
}


CollectableObject* NCL::CSC8503::TutorialGame::AddCollectableToWorld(Vector3 position)
{
	CollectableObject* collectable = new CollectableObject(3);
	SphereVolume* volume = new SphereVolume(0.7f);

	collectable->SetBoundingVolume((CollisionVolume*)volume);

	collectable->GetTransform()
		.SetScale(Vector3(0.7f, 0.7f, 0.7f))
		.SetPosition(position);

	collectable->SetRenderObject(new RenderObject(&collectable->GetTransform(), sphereMesh, nullptr, basicShader));
	collectable->SetPhysicsObject(new PhysicsObject(&collectable->GetTransform(), collectable->GetBoundingVolume()));

	collectable->GetPhysicsObject()->SetInverseMass(0);
	collectable->GetPhysicsObject()->InitSphereInertia();
	collectable->GetRenderObject()->SetColour(Vector4(1, 0.9f, 0, 1));

	world->AddGameObject(collectable);

	return collectable;
}

void NCL::CSC8503::TutorialGame::AddDoorsToWorld()
{
	DoorKeyObject* key_1 = AddKeyToTheWorld(Vector3(0, 11, 60), 2);
	GameObject* door_1 = AddCubeToWorld(Vector3(20, 5, 80), Vector3(10, 3, 1), 0);
	key_1->GetRenderObject()->SetColour(Debug::MAGENTA);
	door_1->GetRenderObject()->SetColour(Debug::MAGENTA);
	key_1->SetLinkedDoor(door_1);
	key_1->SetCurrentGameWorld(world);

	DoorKeyObject* key_2 = AddKeyToTheWorld(Vector3(40, 5, 140), 2);
	GameObject* door_2 = AddCubeToWorld(Vector3(80, 5, 132), Vector3(10, 3, 1), 0);
	key_2->GetRenderObject()->SetColour(Debug::CYAN);
	door_2->GetRenderObject()->SetColour(Debug::CYAN);
	key_2->SetLinkedDoor(door_2);
	key_2->SetCurrentGameWorld(world);

	DoorKeyObject* key_3 = AddKeyToTheWorld(Vector3(120, 5, 160), 2);
	GameObject* door_3 = AddCubeToWorld(Vector3(112, 5, 120), Vector3(1, 3, 10), 0);
	key_3->GetRenderObject()->SetColour(Debug::WHITE);
	door_3->GetRenderObject()->SetColour(Debug::WHITE);
	key_3->SetLinkedDoor(door_3);
	key_3->SetCurrentGameWorld(world);

	DoorKeyObject* key_4 = AddKeyToTheWorld(Vector3(400, 5, 140), 2);
	GameObject* door_4 = AddCubeToWorld(Vector3(100, 5, 108), Vector3(10, 3, 1), 0);
	key_4->GetRenderObject()->SetColour(Debug::BLACK);
	door_4->GetRenderObject()->SetColour(Debug::BLACK);
	key_4->SetLinkedDoor(door_4);
	key_4->SetCurrentGameWorld(world);
}


void NCL::CSC8503::TutorialGame::AddBallPitToWorld()
{
	float sphereSize = 2.5f;
	for (int x = 35; x <= 65; x+=5)
	{
		for (int z = 95; z <= 125; z+=5)
		{
			AddSphereToWorld(Vector3(x, 5, z), sphereSize);
		}
	}
}


StateGameObject* NCL::CSC8503::TutorialGame::AddStateObjectToWorld(const Vector3 position)
{
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

EnemyStateObject* NCL::CSC8503::TutorialGame::AddEnemyStateObjectToWorld(const Vector3 position)
{
	EnemyStateObject* apple = new EnemyStateObject(player, 5);

	SphereVolume* volume = new SphereVolume(1.0f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(1, 1, 1))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), sphereMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}



void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -20, 0), Vector3(200, 2, 200));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld();
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0), Vector3(200, 2, 200));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
				//AddOBBCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}



/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyCodes::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		//Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::Left)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyCodes::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					Window::GetWindow()->ShowOSPointer(true);
					Window::GetWindow()->LockMouseToWindow(false);
					lockedObject = nullptr;
				}
				else {
					Window::GetWindow()->ShowOSPointer(false);
					Window::GetWindow()->LockMouseToWindow(true);
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		//Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	//Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(5, 90));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::Right)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}


Vector3 TutorialGame::GetWorldForwardAxis()
{
	Matrix4 view = world->GetMainCamera().BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	return Vector3::Cross(Vector3(0, 1, 0), rightAxis);
}


int NCL::CSC8503::TutorialGame::GetHighScoreFromFile()
{
	std::ifstream file(Assets::DATADIR + "HighScores.txt");
	int highest = 0;
	int currentNumber;

	if (!file.is_open())
	{
		std::cout << "Couldn't open file\n";
		return -1;
	}

	while (file >> currentNumber)
	{
		if (currentNumber > highest)
		{
			highest = currentNumber;
		}
	}
	file.close();
	return highest;
}

void NCL::CSC8503::TutorialGame::AddScoreToFile(int newScore)
{
	std::ofstream file;
	file.open(Assets::DATADIR + "HighScores.txt", std::ios_base::app);

	if (!file.is_open())
	{
		std::cout << "Couldn't open file to write to\n";
		return;
	}

	file << newScore << "\n";
	file.close();
}