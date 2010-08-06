#include "NSM_Common.h"

#include "zlib.h"

Client *createGlobalClient();

void deleteGlobalClient();

#define MAX_COMPRESSED_OUTBUF_SIZE (1024*1024*64)

class Client
{
protected:
	boost::asio::io_service *io_service;
	tcp::socket *clientSocket;
	tcp::resolver *resolver;

	vector<MemoryBlock> blocks,staleBlocks,xorBlocks;
    vector< string > inputBufferQueue;
    vector<unsigned char> incomingMsg;

    vector<MemoryBlock> constBlocks;

public:
	Client();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

	void initializeConnection(const char *hostname,const char *port);

    bool hasConnection()
    {
        return clientSocket!=NULL;
    }

	bool syncAndUpdate();

	void checkMatch(Server *server);

	int getID()
	{
		return 1;
	}

    void sendString(const string &outputString);

	MemoryBlock getMemoryBlock(int i)
	{
		return blocks[i];
	}

    int getNumConstBlocks()
    {
        return int(constBlocks.size());
    }

    MemoryBlock* getConstBlock(int i)
    {
        return &constBlocks[i];
    }

    string popInputBuffer()
    {
        if(inputBufferQueue.size())
        {
            string retval = inputBufferQueue.back();
            inputBufferQueue.pop_back();
            return retval;
        }
        return "";
    }
};
