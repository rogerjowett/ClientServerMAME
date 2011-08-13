#include "NSM_Common.h"

#include "zlib.h"

Server *createGlobalServer(string _username,unsigned short _port);

void deleteGlobalServer();

class running_machine;

class Server : public Common
{
protected:
    //vector<Session> sessions;

	vector<MemoryBlock> initialBlocks;

    int port;

	bool firstSync;

	list<pair<unsigned char *,int> > syncPacketQueue;

	int syncTransferSeconds;

	RakNet::TimeUS syncTime;

    std::vector<RakNet::RakNetGUID> acceptedPeers;
    std::map<RakNet::SystemAddress,std::vector<RakNet::RakNetGUID> > waitingForAcceptFrom;
    int maxPeerID;
    std::map<RakNet::SystemAddress,int> deadPeerIDs;
    std::map<RakNet::SystemAddress,string> candidateNames;

    bool syncHappend;

public:
    Server() {}

	Server(string _username,int _port);

    void shutdown();

    void acceptPeer(RakNet::SystemAddress saToAccept,running_machine *machine);

    void removePeer(RakNet::RakNetGUID guid,running_machine *machine);

	bool initializeConnection();

	MemoryBlock createMemoryBlock(int size);

	vector<MemoryBlock> createMemoryBlock(unsigned char* ptr,int size);

	void initialSync(const RakNet::SystemAddress &sa,running_machine *machine);

	void update(running_machine *machine);

	void sync();

    void popSyncQueue();

	void setSyncTransferTime(int _syncTransferSeconds)
	{
		syncTransferSeconds = _syncTransferSeconds;
	}

    void sendInputs(const string &inputString);
};

