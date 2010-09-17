#ifndef __NSM_COMMON__
#define __NSM_COMMON__

//RAKNET MUST COME FIRST, OTHER LIBS TRY TO REPLACE new/delete/malloc/free WITH THEIR OWN SHIT
//for ID_USER_PACKET_ENUM
#include "MessageIdentifiers.h"

//for guid, systemaddress, etc.
#include "RakNetTypes.h"

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
    ID_CLIENT_INPUTS=ID_USER_PACKET_ENUM,
    ID_SERVER_INPUTS,
    ID_INITIAL_SYNC,
    ID_RESYNC,
    ID_CONST_DATA,
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

#endif
