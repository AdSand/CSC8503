#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

TutorialGame* g;


vector<Vector3> testNodes;
void TestPathfinding() {
	//NavigationGrid grid("TestGrid1.txt");
	NavigationGrid grid("WorldGrid.txt");

	NavigationPath outPath;

	Vector3 startPos(200, 0, 20);
	Vector3 endPos(60, 0, 0);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos))
	{
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); i++)
	{
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];


		Debug::DrawLine(a, b, Debug::MAGENTA);
	}
}

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void
		{
			std::cout << "I'm in state A!\n";
			data++;
		}
	);

	State* B = new State([&](float dt)->void
		{
			std::cout << "I'm in state B!\n";
			data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool
		{
			return data > 10;
		}
	);

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool
		{
			return data < 0;
		}
	);

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; i++)
	{
		testMachine->Update(1.0f);
	}
}

void TestBehaviourTree()
{
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				std::cout << "looking for a key\n";
				behaviourTimer = rand() % 100;
				state = Ongoing;
			}
			else if (state == Ongoing)
			{
				behaviourTimer -= dt;
				if (behaviourTimer <= 0.0f)
				{
					std::cout << "Found a key!\n";
					return Success;
				}
			}
			return state; // will be ongoing until success
		}
	);

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				std::cout << "Going to loot the room\n";
				state = Ongoing;
			}
			else if (state == Ongoing)
			{
				distanceToTarget -= dt;
				if (distanceToTarget <= 0.0f)
				{
					std::cout << "Reached Room!\n";
					return Success;
				}
			}
			return state; // will be ongoing until success
		}
	);

	BehaviourAction* openDoor = new BehaviourAction("Open Door",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				std::cout << "Opening Door\n";
				return Success;
			}
			return state;
		}
	);

	BehaviourAction* lookForTreasure = new BehaviourAction("Look For Treasure",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				std::cout << "looking for treasure\n";
				return Ongoing;
			}
			else if (state == Ongoing)
			{
				bool found = rand() % 2;
				if (found)
				{
					std::cout << "I found some treasure!\n";
					return Success;
				}
				std::cout << "No treasure here!\n";
				return Failure;
			}
			return state;
		}
	);

	BehaviourAction* lookForItems = new BehaviourAction("Look For Items",
		[&](float dt, BehaviourState state)->BehaviourState
		{
			if (state == Initialise)
			{
				std::cout << "looking for items\n";
				return Ongoing;
			}
			else if (state == Ongoing)
			{
				bool found = rand() % 2;
				if (found)
				{
					std::cout << "I found some items!\n";
					return Success;
				}
				std::cout << "No items in here...\n";
				return Failure;
			}
			return state;
		}
	);

	BehaviourSequence* sequence = new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection = new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; i++)
	{
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're going on an adventure\n";
		while (state == Ongoing)
		{
			state = rootSequence->Execute(1.0f); // fake dt
		}
		if (state == Success)
		{
			std::cout << "What a success adventure!\n";
		}
		else if (state == Failure)
		{
			std::cout << "What a waste of time!\n";
		}
	}
	std::cout << "All done!\n";
}





class PauseScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U))
		{
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		g->UpdatePause(0);
		std::cout << "We have the game paused.\n";
		std::cout << "Press U to unpause.\n\n";
	}
};

class EndScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U))
		{
			g->ResetGame();
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		g->UpdateEndScreen(0);
		std::cout << "Game is over.\n";
		std::cout << "Press U to play again.\n\n";
	}
};

class GameScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{

		if (Window::GetKeyboard()->KeyDown(KeyCodes::P))
		{
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}

		if (g->isGameOver)
		{
			*newState = new EndScreen();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::F5))
		{
			return PushdownResult::Pop;
		}
		g->UpdateGame(dt);
		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		std::cout << "we are currently in the game\n";
		std::cout << "Press P to pause\n";
		std::cout << "Press F5 to go to the menu\n\n";
	}
};

class ServerScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{

		if (Window::GetKeyboard()->KeyDown(KeyCodes::F5))
		{
			return PushdownResult::Pop;
		}
		g->UpdateAsServer(dt);
		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		std::cout << "We have a server\n";
	}
};

class ClientScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{
		if (Window::GetKeyboard()->KeyDown(KeyCodes::P))
		{
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}

		if (g->isGameOver)
		{
			*newState = new EndScreen();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::F5))
		{
			return PushdownResult::Pop;
		}

		g->UpdateAsClient(dt);
		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		std::cout << "We have a client\n";
	}
};

class IntroScreen : public PushdownState
{
	PushdownResult OnUpdate(float dt, PushdownState** newState) override
	{
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE))
		{
			*newState = new GameScreen();
			g->InitialiseAsSinglePlayer();
			g->ResetGame();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::S))
		{
			*newState = new ServerScreen();
			g->InitialiseAsServer();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::C))
		{
			*newState = new ClientScreen();
			g->InitialiseAsClient();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE))
		{
			return PushdownResult::Pop;
		}

		return PushdownResult::NoChange;
	}

	void OnAwake() override
	{
		if (!g) g = new TutorialGame();
		g->UpdateMenu(0);
		std::cout << "Press space to start the game.\n";
		std::cout << "Press s to start the game as server.\n";
		std::cout << "Press c to start the game as client.\n";
		std::cout << "Press escape to exit the game.\n\n";
	}
};

void TestPushdownAutomata(Window* w)
{
	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow())
	{
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (!machine.Update(dt))
		{
			return;
		}
	}
}


class TestPacketReceiver : public PacketReceiver {
public:
	TestPacketReceiver(std::string name)
	{
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source)
	{
		if (type == String_Message)
		{
			StringPacket* realPacket = (StringPacket*)payload;

			std::string msg = realPacket->GetStringFromData();

			std::cout << name << " recieved message " << msg << std::endl;
		}
	}

protected:
	std::string name;
};

void TestNetworking()
{
	NetworkBase::Initialise();

	TestPacketReceiver serverReceiver("Server");
	TestPacketReceiver clientReceiver("Client");

	int port = NetworkBase::GetDefaultPort();

	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server->RegisterPacketHandler(String_Message, &serverReceiver);
	client->RegisterPacketHandler(String_Message, &clientReceiver);

	bool canConnect = client->Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; i++)
	{
		StringPacket p1 = "Server says hello! " + std::to_string(i);
		server->SendGlobalPacket(p1);

		StringPacket p2 = "Client says hello!" + std::to_string(i);
		client->SendPacket(p2);

		server->UpdateServer();
		client->UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	NetworkBase::Destroy();
}


/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);
	//Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1920, 1200, true);
	//TestPushdownAutomata(w);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);
	//TutorialGame* game = new TutorialGame();

	w->GetTimer().GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!

	//TestPathfinding();
	//TestBehaviourTree();
	//TestNetworking();
	PushdownMachine machine(new IntroScreen());

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::F8)) {
			w->SetWindowPosition(0, 0);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
			w->SetWindowPosition(400, 400);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		//DisplayPathfinding();

		//game->UpdateGame(dt);
		machine.Update(dt);

	}
	Window::DestroyGameWindow();
}