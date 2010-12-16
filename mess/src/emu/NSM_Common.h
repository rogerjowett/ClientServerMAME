#ifndef __NSM_COMMON__
#define __NSM_COMMON__

//RAKNET MUST COME FIRST, OTHER LIBS TRY TO REPLACE new/delete/malloc/free WITH THEIR OWN SHIT
//for ID_USER_PACKET_ENUM
#include "MessageIdentifiers.h"

//for guid, systemaddress, etc.
#include "RakNetTypes.h"

//For RakNet::GetTimeUS
#include "GetTime.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <cstring>

#include "zlib.h"

using namespace std;

enum OrderingChannelType
{
    ORDERING_CHANNEL_CLIENT_INPUTS,
    ORDERING_CHANNEL_SERVER_INPUTS,
    ORDERING_CHANNEL_SYNC,
    ORDERING_CHANNEL_CONST_DATA,
    ORDERING_CHANNEL_END
};

enum CustomPacketType
{
    ID_SERVER_INPUTS=ID_USER_PACKET_ENUM,
    ID_CLIENT_INPUTS,
    ID_INITIAL_SYNC_PARTIAL,
    ID_INITIAL_SYNC_COMPLETE,
    ID_RESYNC_PARTIAL,
    ID_RESYNC_COMPLETE,
    ID_CONST_DATA,
    ID_SETTINGS,
    ID_REJECT_NEW_HOST,
	ID_ACCEPT_NEW_HOST,
    ID_HOST_ACCEPTED,
    ID_SEND_PEER_ID,
    ID_CLIENT_INFO,
    ID_END
};

class Client;
class Server;

#define MAX_ZLIB_BUF_SIZE (1024*1024*64)

class MemoryBlock
{
public:
	unsigned char *data;
	int size;

	MemoryBlock()
        :
    data(NULL),
    size(0)
    {
    }

	MemoryBlock(int _size)
		:
	size(_size)
	{
		data = (unsigned char*)malloc(_size);
		memset(data,0,_size);
	}

	MemoryBlock(unsigned char *_data,int _size)
		:
	data(_data),
	size(_size)
	{
	}

	int getBitCount()
	{
		int bitCount=0;
		for(int a=0;a<size;a++)
		{
			for(int bitNum=0;bitNum<7;bitNum++)
			{
				if( (data[a]&(1<<bitNum)) != 0)
				{
					bitCount++;
				}
			}
		}
		return bitCount;
	}
};

class Common
{
protected:
	RakNet::RakPeerInterface *rakInterface;

    int secondsBetweenSync;

    vector<MemoryBlock> constBlocks;

	vector<MemoryBlock> blocks,staleBlocks,xorBlocks;

	z_stream strm;

    int selfPeerID;

    std::map<RakNet::SystemAddress,int> peerIDs;

    string username;
    std::map<int,string> peerNames;

    map<int,vector< string > > peerInputs;
    map<RakNet::SystemAddress,vector< string > > unknownPeerInputs;
    map<int,vector< string > > oldPeerInputs;

    RakNet::TimeUS startupTime;

public:

    Common(string _username);

    int getLargestPing();

    RakNet::SystemAddress ConnectBlocking(const char *defaultAddress, unsigned short defaultPort);

    int getSecondsBetweenSync()
    {
        return secondsBetweenSync;
    }

    void setSecondsBetweenSync(int _secondsBetweenSync);

    void addConstBlock(unsigned char *tmpdata,int size);

	int getNumBlocks()
	{
		return int(blocks.size());
	}

    int getNumConstBlocks()
    {
        return int(constBlocks.size());
    }

    MemoryBlock* getConstBlock(int i)
    {
        return &constBlocks[i];
    }

    void destroyConstBlock(int i)
    {
      constBlocks.erase(constBlocks.begin()+i);
    }

	MemoryBlock getMemoryBlock(int i)
	{
		return blocks[i];
	}

	string getLatencyString(int peerID);

	string getStatisticsString();

    vector<int> getPeerIDs()
    {
        vector<int> retval;
        for(
            std::map<RakNet::SystemAddress,int>::iterator it = peerIDs.begin();
            it != peerIDs.end();
            it++
            )
        {
            retval.push_back(it->second);
        }
        return retval;
    }

    //Note, this doesn't include yourself
    int getNumOtherPeers()
    {
        return int(peerIDs.size())-1;
    }

    int getOtherPeerID(int a);

    string popInputBuffer(int clientIndex);

    inline int getSelfPeerID()
    {
        return selfPeerID;
    }

    inline const string &getPeerNameFromID(int id)
    {
        return peerNames[id];
    }

    inline RakNet::TimeUS getStartupTime()
    {
        return startupTime;
    }

    inline RakNet::TimeUS getTime()
    {
        return RakNet::GetTimeUS();
    }

    inline RakNet::TimeUS getTimeSinceStartup()
    {
        return RakNet::GetTimeUS() - startupTime;
    }
};

#endif
