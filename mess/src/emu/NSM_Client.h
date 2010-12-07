#include "NSM_Common.h"

#include "zlib.h"

Client *createGlobalClient(string _username);

void deleteGlobalClient();

#define MAX_COMPRESSED_OUTBUF_SIZE (1024*1024*64)

class running_machine;

class Client : public Common
{
protected:

	vector<MemoryBlock> syncCheckBlocks;
    vector<unsigned char> incomingMsg;

    bool initComplete;

	unsigned char *syncPtr;

	bool firstResync;

	vector<unsigned char> initialSyncBuffer;

    RakNet::TimeUS timeBeforeSync;

public:
	Client(string _username);

    ~Client();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

	bool initializeConnection(unsigned short selfPort,const char *hostname,unsigned short port,running_machine *machine);

	void updateSyncCheck();

    std::pair<bool,bool> syncAndUpdate();

    void loadInitialData(unsigned char *data,int size);

    bool resync(unsigned char *data,int size);

    void addConstData(unsigned char *data,int size);

	void checkMatch(Server *server);

    inline bool isInitComplete()
    {
	    return initComplete;
    }

    int getNumSessions();

    void sendInputs(const string &inputString);
};
