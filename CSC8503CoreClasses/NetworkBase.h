#pragma once
//#include "./enet/enet.h"
struct _ENetHost;
struct _ENetPeer;
struct _ENetEvent;

enum BasicNetworkMessages {
	None,
	Hello,
	Message,
	String_Message,
	Int_Message,
	HighScore_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Shutdown
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
	}

	GamePacket(short type) : GamePacket() {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct StringPacket : public GamePacket {
	char stringData[256]; // set up the length of the message

	StringPacket(const std::string& message)
	{
		type = BasicNetworkMessages::String_Message; // set the packet type
		size = (short)message.length(); // set the size of the data

		memcpy(stringData, message.data(), size);
	}

	std::string GetStringFromData()
	{
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}
};

struct IntPacket : public GamePacket {
	int x;

	IntPacket(const int intX)
	{
		type = BasicNetworkMessages::Int_Message;
		size = sizeof(IntPacket) - sizeof(GamePacket);

		x = intX;
	}

	int GetIntFromData()
	{
		return x;
	}
};


struct HighScorePacket : public GamePacket {
	int highScore;

	HighScorePacket(const int passedHighScore)
	{
		type = BasicNetworkMessages::HighScore_Message;
		size = size = sizeof(HighScorePacket) - sizeof(GamePacket);

		highScore = passedHighScore;
	}

	int GetHighscoreFromData()
	{
		return highScore;
	}
};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 1234;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}
protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	_ENetHost* netHandle;

	std::multimap<int, PacketReceiver*> packetHandlers; // number of objects mapped to a single key
};