#include "NSM_Server.h"

Server *netServer=NULL;

boost::mutex memoryBlockMutex;

volatile bool memoryBlocksLocked = false;

Server *createGlobalServer(string _port)
{
netServer = new Server(_port);
return netServer;
}

void deleteGlobalServer()
{
if(netServer) delete netServer;
netServer = NULL;
}

Session::Session(shared_ptr<tcp::socket> _activeSocket)
:
activeSocket(_activeSocket)
{
    boost::asio::async_read(*activeSocket,    
        boost::asio::buffer(&incomingMsgLength, sizeof(int)),
        boost::bind(
        &Session::handle_read_header, this,
        boost::asio::placeholders::error));
}

void Session::handle_read_header(const boost::system::error_code& error)
{
    if (!error)
    {
        incomingMsg.clear();
        incomingMsg.resize(incomingMsgLength,0);

        //cout << "GOT MESSAGE FROM CLIENT OF LENGTH: " << incomingMsgLength << endl;

        boost::asio::async_read(*activeSocket,
            boost::asio::buffer(&(incomingMsg[0]), incomingMsgLength),
            boost::bind(&Session::handle_read_body, this,
            boost::asio::placeholders::error));
    }
    else
    {
        netServer->removeSession(this);
    }
}

void Session::handle_read_body(const boost::system::error_code& error)
{
    if (!error)
    {
        string s((const char*)(&(incomingMsg[0])),incomingMsgLength);
        inputBufferQueue.push_back(s);
        incomingMsg.clear();

        //cout << "ADDED BUFFER OF SIZE: " << s.size() << " TO QUEUE\n";

        boost::asio::async_read(*activeSocket,    
            boost::asio::buffer(&incomingMsgLength, sizeof(int)),
            boost::bind(
            &Session::handle_read_header, this,
            boost::asio::placeholders::error));
    }
    else
    {
        netServer->removeSession(this);
    }
}

string Session::popInputBuffer()
{
	if(inputBufferQueue.empty())
	{
		//cout << "INPUT BUFFER QUEUE IS EMPTY\n";
	    return string("");
	}
	//cout << "POPPING A STRING\n";
	string s = inputBufferQueue[0];
	inputBufferQueue.erase(inputBufferQueue.begin());
	return s;
}

z_stream strm;
unsigned char compressedOutBuf[MAX_COMPRESSED_OUTBUF_SIZE];
//unsigned char *compressedOutBufPtr;

z_stream strm2;
unsigned char decompressionBuf[MAX_COMPRESSED_OUTBUF_SIZE];

Server::Server(string _port) 
:
port(_port)
{
	io_service = new boost::asio::io_service();
	endpoint = new tcp::endpoint(tcp::v4(), atoi(port.c_str()) );
	acceptor = new tcp::acceptor(*io_service,*endpoint);

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

}

void Server::initializeConnection()
{
	newSocket = boost::shared_ptr<tcp::socket>(new tcp::socket(*io_service));
	acceptor->async_accept(*(newSocket.get()),
		boost::bind(&Server::handle_accept, this, newSocket, boost::asio::placeholders::error));

	boost::thread t(boost::bind(&boost::asio::io_service::run, io_service));
}

MemoryBlock Server::createMemoryBlock(int size)
{
	boost::mutex::scoped_lock(memoryBlockMutex);
	blocks.push_back(MemoryBlock(size));
	xorBlocks.push_back(MemoryBlock(size));
	staleBlocks.push_back(MemoryBlock(size));
	return blocks.back();
}

MemoryBlock Server::createMemoryBlock(unsigned char *ptr,int size)
{
	boost::mutex::scoped_lock(memoryBlockMutex);
	blocks.push_back(MemoryBlock(ptr,size));
	xorBlocks.push_back(MemoryBlock(size));
	staleBlocks.push_back(MemoryBlock(size));
	return blocks.back();
}

void Server::handle_accept(
				   boost::shared_ptr<tcp::socket> acceptedSocket,
				   const boost::system::error_code& error)
{
    acceptedSocket->set_option(tcp::no_delay(true));
    boost::asio::socket_base::non_blocking_io command(false);
    acceptedSocket->io_control(command); 

	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	boost::mutex::scoped_lock(memoryBlockMutex);
	cout << "IN CRITICAL SECTION\n";
    newSocket = boost::shared_ptr<tcp::socket>(new tcp::socket(*io_service));
	acceptor->async_accept(*(newSocket.get()),
		boost::bind(&Server::handle_accept, this, newSocket, boost::asio::placeholders::error));

	if (!error)
	{
        shared_ptr<Session> newSession;
		try
		{
            newSession = shared_ptr<Session>(new Session(acceptedSocket));
			sessions.push_back(newSession);
			cout << "SERVER: Sending initial snapshot\n";
			int numBlocks = int(blocks.size());
			cout << "NUMBLOCKS: " << numBlocks << endl;
			boost::asio::write(*acceptedSocket,boost::asio::buffer(&numBlocks, sizeof(int)));

			// NOTE: The server must send stale data to the client for the first time
			// So that future syncs will be accurate
			for(int blockIndex=0;blockIndex<int(staleBlocks.size());blockIndex++)
			{
				//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << staleBlocks[blockIndex].size << endl;
				boost::asio::write(*acceptedSocket,boost::asio::buffer(&(staleBlocks[blockIndex].size), sizeof(int)));
				int bytesWritten = boost::asio::write(*acceptedSocket,boost::asio::buffer(staleBlocks[blockIndex].data, staleBlocks[blockIndex].size));
				if(bytesWritten != staleBlocks[blockIndex].size) cout << "OH SHIT! BAD NETWORK SEND\n";
			}

            int numConstBlocks = int(constBlocks.size());
			boost::asio::write(*acceptedSocket,boost::asio::buffer(&numConstBlocks, sizeof(int)));

			// NOTE: The server must send stale data to the client for the first time
			// So that future syncs will be accurate
			for(int blockIndex=0;blockIndex<int(constBlocks.size());blockIndex++)
			{
				//cout << "BLOCK SIZE FOR INDEX " << blockIndex << ": " << constBlocks[blockIndex].size << endl;
                int constBlockSize = int(constBlocks[blockIndex].size);
				boost::asio::write(*acceptedSocket,boost::asio::buffer(&constBlockSize, sizeof(int)));
				int bytesWritten = boost::asio::write(*acceptedSocket,boost::asio::buffer(constBlocks[blockIndex].data, constBlocks[blockIndex].size));
				if(bytesWritten != constBlocks[blockIndex].size) cout << "OH SHIT! BAD NETWORK SEND\n";
			}

            cout << "FINISHED SENDING BLOCKS TO CLIENT\n";
		}
        catch(const std::exception &ex)
		{
			cout << "SERVER: Client closed connection, removing client\n";
            removeSession(newSession.get());
	        memoryBlocksLocked=false;
            return;
		}
	}
	cout << "SERVER: Done with initial snapshot\n";
	cout << "OUT OF CRITICAL AREA\n";
	cout.flush();
	memoryBlocksLocked=false;
}

void Server::syncAndUpdate() 
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	static int runTimes=0;
	runTimes++;
	int bytesSynched=0;
	boost::mutex::scoped_lock(memoryBlockMutex);
	//cout << "IN CRITICAL SECTION\n";
	//cout << "SERVER: Syncing with clients\n";
	bool anyDirty=false;
	for(int blockIndex=0;blockIndex<int(blocks.size());blockIndex++)
	{
		MemoryBlock &block = blocks[blockIndex];
		MemoryBlock &staleBlock = staleBlocks[blockIndex];
		MemoryBlock &xorBlock = xorBlocks[blockIndex];

		if(block.size != staleBlock.size || block.size != xorBlock.size)
		{
			cout << "BLOCK SIZE MISMATCH\n";
		}

		bool dirty=false;
		for(int a=0;a<block.size;a++)
		{
			xorBlock.data[a] = block.data[a] ^ staleBlock.data[a];
			if(xorBlock.data[a]) dirty=true;
		}
		//dirty=true;
		memcpy(staleBlock.data,block.data,block.size);
		if(dirty && !anyDirty)
		{
			//First dirty block
			int ret = deflateInit(&strm, 9/*Z_DEFAULT_COMPRESSION*/);
			strm.next_out = compressedOutBuf;
			strm.avail_out = MAX_COMPRESSED_OUTBUF_SIZE;
			if (ret != Z_OK)
				cout << "CREATING ZLIB STREAM FAILED\n";
			anyDirty=true;
		}

		if(dirty)
		{
			bytesSynched += xorBlock.size;
			//cout << "SERVER: SENDING DIRTY BLOCK AT INDEX: " << blockIndex << endl;
			//Send xorBlock
			//for(int a=0;a<(int)sessions.size();a++)
			{
				//cout << "SENDING DIRTY BLOCK TO CLIENT OF SIZE " << xorBlock.size << "\n";
				try
				{
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&(blockIndex), sizeof(int)));
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&(xorBlock.size), sizeof(int)));
					//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(xorBlock.data, xorBlock.size));
					strm.next_in = (Bytef*)&blockIndex;
		    			strm.avail_in = sizeof(int);
					int retval = deflate(&strm, Z_SYNC_FLUSH);
					strm.next_in = xorBlock.data;
		    			strm.avail_in = xorBlock.size;
					retval = deflate(&strm, Z_SYNC_FLUSH);
					//cout << "DEFLATE RETVAL: " << retval << endl;
					//if(MAX_COMPRESSED_OUTBUF_SIZE - (compressedOutBufPtr-compressedOutBuf) <= 0)
					//{
						//cout << "WE BLEW OUR COMPRESSED OUTPUT BUFFER!\n";
					//}
				}
                		catch(const std::exception &ex)
				{
					//cout << "SERVER: Client closed connection, removing client\n";
					//sessions.erase(sessions.begin()+a);
				}
			}
		}
	}
	if(anyDirty)
	{
	int finishIndex = -1;
	strm.next_in = (Bytef*)&finishIndex;
	strm.avail_in = sizeof(int);
	int retval = deflate(&strm, Z_FINISH);
	//cout << "DEFLATE RETVAL: " << retval << endl;
	for(int a=0;a<(int)sessions.size();a++)
	{
        int header = 1;
        boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&header, sizeof(int)));

		//boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&(finishIndex), sizeof(int)));
		int compressedSize = MAX_COMPRESSED_OUTBUF_SIZE - strm.avail_out;

		char checksum = 0;
		for(int b=0;b<compressedSize;b++)
		{
			checksum = checksum ^ compressedOutBuf[b];
		}
		//cout << "CHECKSUM: " << int(checksum) << endl;

		//cout << "SENDING " << compressedSize << " BYTES\n";
        try
        {
		    boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&compressedSize, sizeof(int)));
		    boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(compressedOutBuf, compressedSize));
        }
        catch(std::exception ex)
        {
            cout << "SERVER: GOT ERROR, CLOSING SESSION\n";
            removeSession(sessions[a].get());
            a--;
        }

#if 0
		/* allocate deflate state */
		strm2.zalloc = Z_NULL;
		strm2.zfree = Z_NULL;
		strm2.opaque = Z_NULL;
		strm2.next_in = compressedOutBuf;
		strm2.avail_in = compressedSize;
		int ret = inflateInit(&strm2);
		strm2.next_out = decompressionBuf;
		strm2.avail_out = MAX_COMPRESSED_OUTBUF_SIZE;
		retval = inflate(&strm2,Z_NO_FLUSH);
		cout << "INFLATE RETVAL: " << retval << endl;
		if(strm2.msg) cout << "ERROR MESSAGE: " << strm2.msg << endl;
		if(retval==Z_DATA_ERROR) exit(1);
#endif
	}
	deflateEnd(&strm);
	}
	//if(runTimes%1==0) cout << "BYTES SYNCED: " << bytesSynched << endl;
	//cout << "OUT OF CRITICAL AREA\n";
	//cout.flush();
	memoryBlocksLocked=false;
}

void Server::sendString(const string &outputString)
{
	boost::mutex::scoped_lock(memoryBlockMutex);
    //cout << "IN CRITICAL SECTION\n";
    try
    {
        int intSize = int(outputString.length());
        cout << "SENDING MESSAGE WITH LENGTH: " << intSize << endl;
	    for(int a=0;a<(int)sessions.size();a++)
	    {
            try
            {
                int header = 2;
	            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&header, sizeof(int)));
	            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&intSize, sizeof(int)));
	            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(outputString.c_str(), intSize));
            }
            catch(std::exception ex)
            {
                cout << "SERVER: GOT ERROR, CLOSING SESSION\n";
                removeSession(sessions[a].get());
                a--;
            }
        }
    }
    catch(std::exception ex)
    {
        cout << "CONNECTION CLOSED\n";
    }
    //cout << "EXITING CRITICAL SECTION\n";
}

void Server::addConstBlock(unsigned char *tmpdata,int size)
{
	while(memoryBlocksLocked)
	{
	;
	}
	memoryBlocksLocked=true;
	boost::mutex::scoped_lock(memoryBlockMutex);
    //cout << "Adding const block...\n";
    //cout << "IN CRITICAL SECTION\n";
    constBlocks.push_back(MemoryBlock(size));
    memcpy(constBlocks.back().data,tmpdata,size);

    for(int a=0;a<(int)sessions.size();a++)
    {
        //cout << "ADDING CONST BLOCK WITH LENGTH: " << size << endl;
        try
        {
            int header = 3;
            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&header, sizeof(int)));
            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(&size, sizeof(int)));
            boost::asio::write(*(sessions[a]->getSocket()),boost::asio::buffer(constBlocks.back().data, size));
        }
        catch(std::exception ex)
        {
            cout << "SERVER: GOT ERROR, CLOSING SESSION\n";
            removeSession(sessions[a].get());
            a--;
        }
    }
	memoryBlocksLocked=false;
    //cout << "done\n";
    //cout << "EXITING CRITICAL SECTION\n";
}


