#include "NSM_Common.h"

#include "zlib.h"

Server *createGlobalServer(string _username,unsigned short _port);

void deleteGlobalServer();

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

    std::vector<RakNet::SystemAddress> acceptedPeers;
    std::map<RakNet::SystemAddress,std::vector<RakNet::SystemAddress> > waitingForAcceptFrom;
    int lastUsedPeerID;
    int maxPeerID;
    std::map<RakNet::SystemAddress,int> deadPeerIDs;

    std::map<RakNet::SystemAddress,string> candidateNames;

public:
	Server(string _username,int _port);

    ~Server();

    void acceptPeer(RakNet::SystemAddress saToAccept);

    void removePeer(RakNet::SystemAddress sa);

	bool initializeConnection();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

	void initialSync(const RakNet::SystemAddress &sa);

	void update();

	void sync();

    void popSyncQueue();

    int getClientID(int i)
    {
	    return i+1;
    }

	void setSyncTransferTime(int _syncTransferSeconds)
	{
		syncTransferSeconds = _syncTransferSeconds;
	}

    void sendInputs(const string &inputString);
};

