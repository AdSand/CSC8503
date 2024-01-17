#include "../NCLCoreClasses/KeyboardMouseController.h"

#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"
#include "HingeConstraint.h"
#include "StateGameObject.h"
#include "EnemyStateObject.h"
#include "DoorKeyObject.h"
#include "HeistObject.h"
#include "PlayerObject.h"
#include "CollectableObject.h"
#include "AStarEnemy.h"

#include "Assets.h"
#include <GameServer.h>
#include <GameClient.h>

class DoorKeyObject;
class HeistObject;
class PlayerObject;
class CollectableObject;
class AStarEnemy;

namespace NCL {
	namespace CSC8503 {
		class TutorialGame : public PacketReceiver		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			void UpdateMenu(float dt);
			void UpdatePause(float dt);
			void UpdateEndScreen(float dt);

			bool isGameOver = false;
			void ResetGame();

			void InitialiseAsSinglePlayer();

			void InitialiseAsServer();
			void UpdateAsServer(float dt);

			void InitialiseAsClient();
			void UpdateAsClient(float dt);

			void ReceivePacket(int type, GamePacket* payload, int source = -1) override;

			bool isClient;

			GameServer* server;
			GameClient* client;

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position, Vector3 floorSize);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddOBBCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddCapsuleToWorld(const Vector3& position, float radius, float halfHeight, float inverseMass = 10.0f);

			PlayerObject* AddPlayerToWorld();
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			StateGameObject* AddStateObjectToWorld(const Vector3 position);
			StateGameObject* testStateObject;

			// ---------------------Coursework extras---------------------

			PlayerObject* player;

			EnemyStateObject* AddEnemyStateObjectToWorld(const Vector3 position);
			EnemyStateObject* enemy1;
			EnemyStateObject* enemy2;

			void BuildFromFile(std::string filename, float height);

			void AddBridgeToWorld(Vector3 startPos);
			void AddGateToWorld(Vector3 startPos);
			void AddBallPitToWorld();
			void AddDoorsToWorld();
			CollectableObject* AddCollectableToWorld(Vector3 position);

			DoorKeyObject* AddKeyToTheWorld(const Vector3& position, float radius);

			HeistObject* AddHeistObjectToWorld(const Vector3& position);
			HeistObject* heistObject;

			float timer;
			int collectablesInWorld;

			AStarEnemy* AStarChaseEnemy;
			AStarEnemy* AddAStarEnemyToWorld(const Vector3& position, float radius);

			Vector3 GetWorldForwardAxis();

			int serverHighScore = NULL;
			int GetHighScoreFromFile();
			void AddScoreToFile(int newScore);

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			KeyboardMouseController controller;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			Mesh*	capsuleMesh = nullptr;
			Mesh*	cubeMesh	= nullptr;
			Mesh*	sphereMesh	= nullptr;

			Texture*	basicTex	= nullptr;
			Shader*		basicShader = nullptr;

			//Coursework Meshes
			Mesh*	charMesh	= nullptr;
			Mesh*	enemyMesh	= nullptr;
			Mesh*	bonusMesh	= nullptr;
			Mesh*	gooseMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 10, 35);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;
		};
	}
}

