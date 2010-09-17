#include "NSM_Common.h"

#include "zlib.h"

Server *createGlobalServer(string _port);

void deleteGlobalServer();

class Session
{
protected:
    vector< string > inputBufferQueue;
    RakNet::RakNetGUID guid;

public:
    Session(const RakNet::RakNetGUID &_guid);

    void pushInputBuffer(const string &s);

    string popInputBuffer();

    const RakNet::RakNetGUID &getGUID() const
    {
	    return guid;
    }

    void setGUID(const RakNet::RakNetGUID &_guid)
    {
	    guid = _guid;
    }
};

class Server
{
protected:
	RakNet::RakPeerInterface *rakInterface;

    vector<Session> sessions;

	vector<MemoryBlock> blocks,staleBlocks,xorBlocks;

    vector<MemoryBlock> constBlocks;

    string port;

public:
	Server(string _port);

    ~Server();

	bool initializeConnection();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

    string popInputBuffer(int clientIndex)
    {
        return sessions[clientIndex].popInputBuffer();
    }

	int getNumBlocks()
	{
		return int(blocks.size());
	}

	MemoryBlock getMemoryBlock(int i)
	{
		return blocks[i];
	}

	void initialSync(const RakNet::RakNetGUID &guid);

	void update();

	void sync();

	int getNumSessions() { return int(sessions.size()); }

    int getSessionIndexFromGUID(const RakNet::RakNetGUID &guid)
	{
		for(int a=0;a<(int)sessions.size();a++)
		{
			if(sessions[a].getGUID()==guid)
			{
				return a;
			}
		}
        return -1;
	}

    void sendString(const string &outputString);

    void addConstBlock(unsigned char *tmpdata,int size);

    int getClientID(int i)
    {
	    return i+1;
    }
	
	string getLatencyString(int connectionIndex);

	string getStatisticsString();
};

