#include "NSM_Common.h"

#include "zlib.h"

Server *createGlobalServer(string _port);

void deleteGlobalServer();

class Session
{
protected:
    shared_ptr<tcp::socket> activeSocket;
    vector< string > inputBufferQueue;
    int incomingMsgLength;
    vector<unsigned char> incomingMsg;

public:
    Session(shared_ptr<tcp::socket> _activeSocket);

    void handle_read_header(const boost::system::error_code& error);

    void handle_read_body(const boost::system::error_code& error);

    shared_ptr<tcp::socket> getSocket()
    {
        return activeSocket;
    }

    string popInputBuffer();
};

#define MAX_COMPRESSED_OUTBUF_SIZE (1024*1024*64)

class Server
{
protected:
	boost::asio::io_service *io_service;
	tcp::endpoint *endpoint;
	tcp::acceptor *acceptor;

    vector<shared_ptr<Session> > sessions;

	vector<MemoryBlock> blocks,staleBlocks,xorBlocks;

    vector<MemoryBlock> constBlocks;

    boost::shared_ptr<tcp::socket> newSocket;

    string port;

public:
	Server(string _port);

	void initializeConnection();

	MemoryBlock createMemoryBlock(int size);

	MemoryBlock createMemoryBlock(unsigned char* ptr,int size);

    string popInputBuffer(int clientIndex)
    {
        return sessions[clientIndex]->popInputBuffer();
    }

	int getNumBlocks()
	{
		return int(blocks.size());
	}

	MemoryBlock getMemoryBlock(int i)
	{
		return blocks[i];
	}

	void handle_accept(
		   boost::shared_ptr<tcp::socket> acceptedSocket,
		   const boost::system::error_code& error
		   );

	void syncAndUpdate();

    void removeSession(Session *session)
    {
        for(int a=0;a<(int)sessions.size();a++)
        {
            if(sessions[a].get()==session)
            {
                sessions.erase(sessions.begin()+a);
                break;
            }
        }
    }

	int getNumSessions() { return int(sessions.size()); }

    void sendString(const string &outputString);

    void addConstBlock(unsigned char *tmpdata,int size);

    int getClientID(int i)
    {
	    //For now, just make the clientID based on the index
	    //This won't work long term, if someone drops out, 
	    //everyone will shift
	    return i+1;
    }
};

